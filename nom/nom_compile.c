#ifndef NOM_COMPILE_C
#define NOM_COMPILE_C

#include "nom_compile.h"

#include <unistd.h>

// Gets a single row of dependencies from a dep file as an array
// Modifies `deps_file`
static NomConstStrDarr internal_nom_parse_deps(char *deps_file) {
    NomConstStrDarr ret = {0};

    if(deps_file == NULL) {
        return ret;
    }

    char *s = deps_file;

    // Find dependencies section after ':'
    for(; *s != 0 && *s != '\n' && *s != ':'; s++) {
        // Escape newline
        if(*s == '\\') {
            s++;
            if(*s == '\n') continue;
        }
    }

    if(*s == 0 || *s == '\n') {
        // No ':' found -> No dependency section
        return ret;
    }

    // Skip ':'
    s++;

    for(; *s != 0 && *s != '\n'; s++) {
        // Ignore spaces
        if(*s == ' ') continue;

        // Escape newline
        if(*s == '\\') {
            s++;
            if(*s == '\n') continue;
        }

        // New dep
        nom_darr_append(&ret, s);

        // Reach end of dep
        for(; *s != 0 && *s != '\n' && *s != ' '; s++);
        if(*s == 0 || *s == '\n') {
            // End of dep is EOF or EOL
            *s = 0;
            return ret;
        }

        // *s == ' ' -> Normal end of dep
        *s = 0;
    }

    return ret;
}

bool nom_needs_rebuild(const char *target_path, const char * const dependencies[], size_t dependencies_count) {
    struct stat statbuf;
    if(stat(target_path, &statbuf) < 0) {
        if(errno == ENOENT) {
            // if output does not exist it must be rebuilt
            return true;
        }
        nom_log(NOM_ERROR, "could not stat `%s`: %s", target_path, strerror(errno));
        return true;
    }
    time_t target_updated_at = statbuf.st_mtime;

    for(size_t i = 0; i < dependencies_count; ++i) {
        const char *dependency = dependencies[i];
        if(stat(dependency, &statbuf) < 0) {
            // non-existing input is an error because it is needed for building in the first place
            nom_log(NOM_ERROR, "could not stat `%s`: %s", dependency, strerror(errno));
            return true;
        }
        time_t dependency_updated_at = statbuf.st_mtime;
        // if dependency is fresher => rebuild
        if(dependency_updated_at > target_updated_at) {
            return true;
        }
    }

    return false;
}

void internal_nom_do_rebuild(int argc, const char **argv, const char *src_path, bool run) {
    const char *binary_path = argv[0];

    NomStringBuilder sb = {0};
    nom_sb_append_str(&sb, binary_path);
    nom_sb_append_str(&sb, ".old");
    nom_sb_append_null(&sb);

    if(!nom_rename(binary_path, sb.items)) {
        nom_sb_free(&sb);
        exit(1);
    }

    NomCmd cmd = {0};
    if(!nom_cmd_run(&cmd, NOM_REBUILD_YOURSELF_CC, "-o", binary_path, NOM_REBUILD_YOURSELF_FLAGS, src_path)) {
        nom_rename(sb.items, binary_path);
        nom_cmd_free(&cmd);
        nom_sb_free(&sb);
        exit(1);
    }
    nom_delete(sb.items);


    if(run) {
        nom_cmd_append_buf(&cmd, argv, argc);
        if(!nom_cmd_run_sync(cmd)) {
            nom_cmd_free(&cmd);
            nom_sb_free(&sb);
            exit(1);
        }
    }

    nom_cmd_free(&cmd);
    nom_sb_free(&sb);
    exit(0);
}

void nom_rebuild_yourself(int argc, const char **argv, const char *src_path) {
#ifdef NOM_ALLOW_FORCE_REBUILD_YOURSELF
    if(argc > 1 && strcmp(argv[1], "nom_rebuild") == 0) {
        internal_nom_do_rebuild(argc, argv, src_path, false);
    }
#endif

    const char *binary_path = argv[0];

    int pipe_fd[2];
    if(pipe(pipe_fd) == -1) {
        exit(1);
    }
    int pipe_read = pipe_fd[0], pipe_write = pipe_fd[1];

    NomCmd cmd = {0};
    cmd.out_fd = pipe_write;
    nom_log_set_level(NOM_ERROR);
    if(!nom_cmd_run(&cmd, NOM_REBUILD_YOURSELF_CC, "-MM", "-o", "-", NOM_REBUILD_YOURSELF_FLAGS, src_path)) {
        nom_log_set_level(NOM_INFO);
        nom_cmd_free(&cmd);
        exit(1);
    }
    nom_log_set_level(NOM_INFO);
    nom_cmd_free(&cmd);
    close(pipe_write);
    NomStringBuilder src_deps_file = nom_read_fd(pipe_read);
    close(pipe_read);

    bool needs_rebuild;
    if(src_deps_file.items == NULL) {
        needs_rebuild = true;
    } else {
        nom_sb_append_null(&src_deps_file);
        NomConstStrDarr src_deps = internal_nom_parse_deps(src_deps_file.items);
        needs_rebuild = nom_needs_rebuild(binary_path, src_deps.items, src_deps.len);
        nom_darr_free(&src_deps);
        nom_sb_free(&src_deps_file);
    }

    if(needs_rebuild) {
        internal_nom_do_rebuild(argc, argv, src_path, true);
    }
}

static bool internal_nom_src_needs_rebuild(const char *obj_path) {
    // Dependency path
    NomStringBuilder deps_path = {0};
    nom_sb_append_str(&deps_path, obj_path);
    deps_path.len -= 2;
    nom_sb_append_str(&deps_path, ".d");
    nom_sb_append_null(&deps_path);

    // Try to find cached deps file
    NomStringBuilder deps_file = nom_read_file(deps_path.items);
    if(deps_file.items == NULL) {
        // No cached deps file, or we got an error while fetching it. Either way we need to rebuild.
        nom_sb_free(&deps_file);
        nom_sb_free(&deps_path);
        return true;
    }

    // Deps file found. Check if object file it's still valid.
    nom_sb_append_null(&deps_file);
    NomConstStrDarr deps = internal_nom_parse_deps(deps_file.items);
    bool ret = nom_needs_rebuild(obj_path, deps.items, deps.len);

    nom_darr_free(&deps);
    nom_sb_free(&deps_file);
    nom_sb_free(&deps_path);
    return ret;
}

typedef struct InternalNomCompileFileState {
    const NomCompileConfig *config;
    NomCmd cmd;
    NomProcs procs;
    NomConstStrDarr objs;
} InternalNomCompileFileState;

static void internal_nom_compile_file(const char *path, NomFileType type, NomFileStats *ftw, InternalNomCompileFileState *state) {
    if(type != NOM_FILE_REG || strcmp(path + ftw->path_len - 2, ".c") != 0) {
        // Only process '.c' files
        return;
    }

    const NomCompileConfig *config = state->config;

    // Dir
    NomStringBuilder obj_path = {0};
    nom_sb_append_str(&obj_path, config->obj_dir);
    nom_sb_append_buf(&obj_path, path + ftw->base_root, ftw->base_name - ftw->base_root - 1);
    nom_sb_append_null(&obj_path);
    nom_mkdir(obj_path.items);

    // Src File
    obj_path.len--;
    nom_sb_append_char(&obj_path, '/');
    nom_sb_append_str(&obj_path, path + ftw->base_name);

    // Obj File
    obj_path.len -= 2;
    nom_sb_append_str(&obj_path, ".o");
    nom_sb_append_null(&obj_path);
    nom_darr_append(&state->objs, obj_path.items);

    // Compile if it needs rebuild
    if(internal_nom_src_needs_rebuild(obj_path.items)) {
        NomCmd cmd = state->cmd;
        nom_cmd_append(&cmd, config->cc, "-c", "-MMD", "-o", obj_path.items);
        nom_cmd_append_flags(&cmd, config->flags);
        nom_cmd_append(&cmd, path);
        nom_darr_append(&state->procs, nom_cmd_run_async(cmd));
        nom_cmd_reset(&cmd);
        state->cmd = cmd;
    }
}

static bool internal_nom_walkable_compile_file(const char *path, NomFileType type, NomFileStats *ftw, va_list args) {
    InternalNomCompileFileState *state = va_arg(args, InternalNomCompileFileState *);

    internal_nom_compile_file(path, type, ftw, state);

    return true;
}

bool nom_compile(const NomCompileConfig *config) {
    bool ret = true;
    const char *obj;

    InternalNomCompileFileState state = {
        .config     = config,
        .cmd        = {0},
        .procs      = {0},
        .objs       = {0},
    };

    if(!nom_mkdir(config->obj_dir)) nom_return_defer(false);

    if(!nom_files_walk_tree(config->src_dir, internal_nom_walkable_compile_file, &state)) nom_return_defer(false);
    if(!nom_procs_wait(state.procs)) nom_return_defer(false);

    // Only link if any object file changed (or executable doesn't exist)
    if(nom_needs_rebuild(config->target, state.objs.items, state.objs.len)) {
        NomCmd link_cmd = state.cmd;
        nom_cmd_append(&link_cmd, config->cc, "-o", config->target);
        nom_cmd_append_flags(&link_cmd, config->flags);
        nom_cmd_append_buf(&link_cmd, state.objs.items, state.objs.len);
        if(!nom_cmd_run_sync(link_cmd)) nom_return_defer(false);
        nom_cmd_reset(&link_cmd);
        state.cmd = link_cmd;
    }

defer:
    // Free Compile File State
    nom_cmd_free(&state.cmd);
    nom_darr_free(&state.procs);
    nom_darr_foreach(obj, state.objs) {
        NOM_FREE_CONST(obj);
    }
    nom_darr_free(&state.objs);

    return ret;
}

static void internal_nom_build_compile_object(const char *path, NomFileType type, NomFileStats *ftw, const NomCompileConfig *config, const char *cwd, NomStringBuilder *out) {
    #define INDENT "    "

    if(type != NOM_FILE_REG || strcmp(path + ftw->path_len - 2, ".c") != 0) {
        // Only process '.c' files
        return;
    }

    // Open
    nom_sb_append_str(out, INDENT "{\n");

    // Directory
    nom_sb_append_str(out, INDENT INDENT "\"directory\": \"");
    nom_sb_append_str(out, cwd);
    nom_sb_append_str(out, "\",\n");

    // File
    nom_sb_append_str(out, INDENT INDENT "\"file\": \"");
    nom_sb_append_str(out, path);
    nom_sb_append_str(out, "\",\n");

    // Arguments
    nom_sb_append_str(out, INDENT INDENT "\"arguments\": [\"");

    nom_sb_append_str(out, config->cc);
    nom_sb_append_str(out, "\", \"-c\", \"-o\", \"");
    nom_sb_append_str(out, config->obj_dir);
    nom_sb_append_str(out, path + ftw->base_root);
    out->len -= 2;
    nom_sb_append_str(out, ".o\", ");

    NomCmdFlags flags = config->flags;
    const char *arg;
    nom_darr_foreach(arg, flags) {
        nom_sb_append_char(out, '"');
        nom_sb_append_str(out, arg);
        nom_sb_append_str(out, "\", ");
    }

    nom_sb_append_char(out, '"');
    nom_sb_append_str(out, path);
    nom_sb_append_str(out, "\"]\n");

    // Close
    nom_sb_append_str(out, INDENT "},\n");

    #undef INDENT
}

static bool internal_nom_walkable_build_compile_object(const char *path, NomFileType type, NomFileStats *ftw, va_list args) {
    const NomCompileConfig *config = va_arg(args, NomCompileConfig *);
    const char *cwd = va_arg(args, const char *);
    NomStringBuilder *out = va_arg(args, NomStringBuilder *);

    internal_nom_build_compile_object(path, type, ftw, config, cwd, out);

    return true;
}

bool nom_build_compilation_database(const NomCompileConfig *config) {
    const char *src_dir = config->src_dir;
    // TODO: Support absolute paths
    NOM_ASSERT(*src_dir != '/' && "we don't support absolute paths for now");

    char *cwd = nom_get_cwd();
    if(!cwd) return false;

    NomStringBuilder compile_db_sb = {0};
    nom_sb_append_str(&compile_db_sb, "[\n");
    nom_files_walk_tree(src_dir, internal_nom_walkable_build_compile_object, config, cwd, &compile_db_sb);
    compile_db_sb.len -= 2; // Delete trailing comma
    nom_sb_append_str(&compile_db_sb, "\n]\n");
    bool ret = nom_write_file("compile_commands.json", nom_sb_to_sv(compile_db_sb));

    nom_sb_free(&compile_db_sb);
    NOM_FREE(cwd);
    return ret;
}

bool nom_clean(const NomCompileConfig *compile_config) {
    bool ret = true;
    ret &= nom_delete(compile_config->obj_dir);
    ret &= nom_delete(compile_config->target);
    return ret;
}

#endif //NOM_COMPILE_C
