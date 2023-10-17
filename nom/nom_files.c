#ifndef NOM_FILES_C
#define NOM_FILES_C

#include "nom_files.h"

#include "nom_defs.h"
#include "nom_log.h"
#include "nom_sb.h"
#include "nom_dequeue.h"

#include <dirent.h>
#include <errno.h>

static bool internal_nom_stat(const char *path, struct stat *statbuf) {
    if(stat(path, statbuf) < 0) {
        nom_log(NOM_ERROR,"stat on `%s` failed: %s", path, strerror(errno));
        return false;
    }
    return true;
}

static NomFileType internal_nom_file_type_from_stat_mode(mode_t st_mode) {
    switch (st_mode & S_IFMT) {
        case S_IFREG:  return NOM_FILE_REG;
        case S_IFDIR:  return NOM_FILE_DIR;
        default:       return NOM_FILE_OTHER;
    }
    NOM_ASSERT(0 && "unreachable");
}

NomFileType nom_file_type(const char *path) {
    struct stat statbuf;
    if(!internal_nom_stat(path, &statbuf)) return NOM_FILE_FAILED;
    return internal_nom_file_type_from_stat_mode(statbuf.st_mode);
}

const char *nom_file_type_str(NomFileType file_type) {
    switch(file_type) {
        case NOM_FILE_FAILED:   return "failed";
        case NOM_FILE_REG:      return "regular";
        case NOM_FILE_DIR:      return "directory";
        case NOM_FILE_OTHER:    return "other";
    }
    NOM_ASSERT(0 && "unreachable");
}

static NomFileStats internal_nom_fwt_build(NomStringBuilder path_sb, size_t root_level, size_t base_root) {
    const char *path = path_sb.items;
    size_t base_name = 0;
    size_t level = -root_level;

    for(const char *s = path; *s; ++s) {
        if(*s == '/') {
            base_name = s - path + 1;
            level++;
        }
    }

    NomFileStats ret = {
        .path_len = path_sb.len - 1,
        .base_root = base_root,
        .base_name = base_name,
        .level = level,
    };
    return ret;
}

bool nom_files_walk_tree(const char *root_dir, bool (*file_callback)(const char *path, NomFileType type, NomFileStats *ftw, va_list args), ...) {
    if(!nom_file_exists(root_dir)) {
        return true;
    }

    bool success = true;
    bool keep_going = true;
    va_list args;
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    NomDeq(NomStringBuilder) q = {0};

    NomStringBuilder root_sb = {0};
    nom_sb_append_str(&root_sb, root_dir);
    if(nom_sb_last(root_sb) == '/') {
        root_sb.len--;
    }
    nom_sb_append_null(&root_sb);
    nom_deq_push_r(&q, root_sb);

    size_t base_root = root_sb.len - 1;
    NomFileStats base_ftw = internal_nom_fwt_build(root_sb, 0, base_root);
    size_t base_level = base_ftw.level;

    while(!nom_deq_is_empty(q)) {
        NomStringBuilder sb = nom_deq_pop_r(&q);

        if(!internal_nom_stat(sb.items, &statbuf)) {
            success = false;
            nom_sb_free(&sb);
            continue;
        }
        NomFileType file_type = internal_nom_file_type_from_stat_mode(statbuf.st_mode);

        NomFileStats ftw = internal_nom_fwt_build(sb, base_level, base_root);
        ftw.stat = &statbuf;
        va_start(args, file_callback);
        keep_going = file_callback(sb.items, file_type, &ftw, args);
        va_end(args);

        if(!keep_going) {
            nom_sb_free(&sb);
            break;
        }

        if(file_type != NOM_FILE_DIR) {
            nom_sb_free(&sb);
            continue;
        }

        if((dp = opendir(sb.items)) == NULL) {
            success = false;
            nom_log(NOM_ERROR,"cannot open directory `%s`: %s", sb.items, strerror(errno));
            nom_sb_free(&sb);
            continue;
        }

        while((errno = 0, entry = readdir(dp)) != NULL) {
            if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                // Skip . and ..
                continue;
            }

            NomStringBuilder new_sb = {0};
            nom_sb_append_sb(&new_sb, sb);
            new_sb.len--;
            nom_sb_append_char(&new_sb, '/');
            nom_sb_append_str(&new_sb, entry->d_name);
            if(nom_sb_last(new_sb) == '/') {
                // No trailing slash
                new_sb.len--;
            }
            nom_sb_append_null(&new_sb);

            nom_deq_push_r(&q, new_sb);
        }
        if(errno) {
            success = false;
            nom_log(NOM_ERROR,"cannot read directory `%s`: %s", sb.items, strerror(errno));
        }

        closedir(dp);
        nom_sb_free(&sb);
    }

    // Free Queue
    while(!nom_deq_is_empty(q)) {
        NomStringBuilder sb = nom_deq_pop_r(&q);
        nom_sb_free(&sb);
    }
    nom_deq_free(&q);

    return success && keep_going;
}

bool nom_files_read_dir(const char *dir, bool (*file_callback)(const char *path, NomFileType type, NomFileStats *ftw, va_list args), ...) {
    if(!nom_file_exists(dir)) {
        return true;
    }

    bool success = true;
    bool keep_going = true;
    va_list args;
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    NomStringBuilder sb = {0};
    nom_sb_append_str(&sb, dir);
    if(nom_sb_last(sb) != '/') {
        nom_sb_append_char(&sb, '/');
    }
    size_t sb_root_checkpoint = sb.len;

    if((dp = opendir(dir)) == NULL) {
        success = false;
        nom_log(NOM_ERROR, "cannot open directory `%s`: %s", dir, strerror(errno));
        nom_sb_free(&sb);
        return false;
    }
    while(errno = 0, keep_going && (entry = readdir(dp)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            // Skip . and ..
            continue;
        }

        nom_sb_append_str(&sb, entry->d_name);
        if(nom_sb_last(sb) == '/') {
            // No trailing slash
            sb.len--;
        }
        nom_sb_append_null(&sb);

        if(!internal_nom_stat(sb.items, &statbuf)) {
            success = false;
            sb.len = sb_root_checkpoint;
            continue;
        }
        NomFileType file_type = internal_nom_file_type_from_stat_mode(statbuf.st_mode);

        NomFileStats ftw = {
            .path_len   = sb.len - 1,
            .base_root  = sb_root_checkpoint - 1,
            .base_name  = sb_root_checkpoint,
            .level      = 1,
            .stat       = &statbuf,
        };

        va_start(args, file_callback);
        keep_going = file_callback(sb.items, file_type, &ftw, args);
        va_end(args);

        sb.len = sb_root_checkpoint;
    }
    if(errno) {
        success = false;
        nom_log(NOM_ERROR, "cannot read directory `%s`: %s", dir, strerror(errno));
    }

    closedir(dp);
    nom_sb_free(&sb);

    return success && keep_going;
}

bool nom_mkdir(const char *path) {
    static const mode_t mode =
        S_IRWXU                 // Owner: Read, Write, Execute
        | S_IRGRP | S_IXGRP     // Group: Read, Execute
        | S_IROTH | S_IXOTH     // Others: Read, Execute
        ;

    // Fast Path -  Try to create dir without allocating any memory
    if(mkdir(path, mode) < 0) {
        if(errno == EEXIST) {
            // Something already exists
            NomFileType file_type = nom_file_type(path);
            if(!file_type) return false;

            if(file_type == NOM_FILE_DIR) {
                // Dir already exists
                return true;
            }

            nom_log(NOM_ERROR, "could not create directory `%s`: another file of `%s` type with same name already exists", path, nom_file_type_str(file_type));
            return false;
        } else if(errno == ENOENT) {
            // Subdir doesn't exist
            // Continue to path were we create subdirs recursively
        } else {
            nom_log(NOM_ERROR, "could not create directory `%s`: %s", path, strerror(errno));
            return false;
        }
    } else {
        nom_log(NOM_INFO, "created directory `%s`", path);
        return true;
    }

    bool ret = false;

    NomStringBuilder path_sb = {0};
    nom_sb_append_str(&path_sb, path);
    if(nom_sb_last(path_sb) == '/') {
        path_sb.items[path_sb.len - 1] = 0;
    } else {
        nom_sb_append_null(&path_sb);
    }

    size_t dirs_to_create = 1;

    size_t i;
    for(i = path_sb.len - 2; i > 0 && path_sb.items[i] != '/'; --i);
    NOM_ASSERT(i > 0 && "dir has no parent folder");
    path_sb.items[i] = 0;
    path_sb.len = i + 1;
    dirs_to_create++;

    while(dirs_to_create) {
        if(mkdir(path_sb.items, mode) < 0) {
            if(errno == EEXIST) {
                // Something already exists
                NomFileType file_type = nom_file_type(path);
                if(!file_type) nom_return_defer(false);

                NOM_ASSERT(file_type != NOM_FILE_DIR && "subdirectory didn't exist, but now does -_-");
                nom_log(NOM_ERROR, "could not create directory `%s`: another file of `%s` type with same name already exists", path_sb.items, nom_file_type_str(file_type));
                nom_return_defer(false);

            } else if(errno == ENOENT) {
                // Subdir doesn't exist
                for(i = path_sb.len - 2; i > 0 && path_sb.items[i] != '/'; --i);
                NOM_ASSERT(i > 0 && "dir has no parent folder");
                path_sb.items[i] = 0;
                path_sb.len = i + 1;
                dirs_to_create++;
                continue;

            } else {
                nom_log(NOM_ERROR, "could not create directory `%s`: %s", path_sb.items, strerror(errno));
                nom_return_defer(false);
            }
        }

        nom_log(NOM_INFO, "created directory `%s`", path_sb.items);
        dirs_to_create--;

        if(dirs_to_create) {
            path_sb.items[path_sb.len-1] = '/';
            for(i = path_sb.len; path_sb.items[i]; ++i);
            path_sb.len = i + 1;
        }
    }

    nom_return_defer(true);

defer:
    nom_sb_free(&path_sb);
    return ret;
}

static NomStringBuilder internal_nom_read_file(const char *path, int fd) {
    NOM_ASSERT(!(path && fd) && "only use path or fd");
    static const uint buf_size = 32*1024;

    NomStringBuilder ret = {0};
    size_t cap = 0;
    char *items = NULL;

    const char *modes = "rb";
    FILE *f = path ? fopen(path, modes) : fdopen(fd, modes);
    if(f == NULL) {
        if(errno != ENOENT) {
            if(path) {
                nom_log(NOM_ERROR, "Could not open `%s` for reading: %s", path, strerror(errno));
            } else {
                nom_log(NOM_ERROR, "Could not open file descriptor `%d` for reading: %s", fd, strerror(errno));
            }
        }
        return ret;
    }

    size_t n, len = 0;
    do {
        cap += buf_size;
        char *tmp = NOM_REALLOC(items, (cap + 1) * sizeof(*items));
        NOM_ASSERT(tmp != NULL && "realloc failed");
        items = tmp;
        n = fread(items + len, 1, buf_size, f);
        len += n;
    } while(n == buf_size);

    if(ferror(f)) {
        if(path) {
            nom_log(NOM_ERROR, "Could not read `%s`: %s\n", path, strerror(errno));
        } else {
            nom_log(NOM_ERROR, "Could not read file descriptor `%d`: %s\n", fd, strerror(errno));
        }
    } else {
        ret.items = items;
        ret.len = len;
        ret.cap = cap;
    }

    fclose(f);
    return ret;
}

NomStringBuilder nom_read_file(const char *path) {
    return internal_nom_read_file(path, 0);
}

NomStringBuilder nom_read_fd(int fd) {
    return internal_nom_read_file(NULL, fd);
}

bool nom_write_file(const char *path, NomStringView data) {
    bool ret = true;

    FILE *f = fopen(path, "wb");
    if(f == NULL) {
        nom_log(NOM_ERROR, "Could not open or create file `%s` for writing: %s", path, strerror(errno));
        return false;
    }

    const char *buf = data.data;
    size_t size = data.len;
    while(size > 0) {
        size_t n = fwrite(buf, 1, size, f);
        if(ferror(f)) {
            nom_log(NOM_ERROR, "Could not write into file `%s`: %s", path, strerror(errno));
            nom_return_defer(false);
        }
        size -= n;
        buf  += n;
    }

    nom_log(NOM_INFO, "created file `%s`", path);

defer:
    fclose(f);
    return ret;
}

char *nom_get_cwd(void) {
    size_t size = PATH_MAX;
    char *ret = NOM_MALLOC(sizeof(*ret)*size);
    NOM_ASSERT(ret != NULL && "malloc failed");
    while(!getcwd(ret, size)) {
        if(errno == ERANGE) {
            size *= 2;
            char *tmp = NOM_REALLOC(ret, sizeof(*ret)*size);
            NOM_ASSERT(tmp != NULL && "realloc failed");
            ret = tmp;
        } else {
            nom_log(NOM_ERROR, "Could not get cwd: %s", strerror(errno));
            NOM_FREE(ret);
            return NULL;
        }
    }
    return ret;
}

bool nom_file_exists(const char *file_path) {
    struct stat statbuf;
    if(stat(file_path, &statbuf) < 0) {
        if(errno == ENOENT) {
            return false;
        }
        nom_log(NOM_ERROR, "Could not check if file %s exists: %s", file_path, strerror(errno));
        return false;
    }
    return true;
}

static bool internal_nom_delete_dir_elem(const char *path, NomFileType type, NomFileStats *ftw, NomStrDeq *dirs) {
    (void) ftw; // unused

    switch(type) {
        case NOM_FILE_REG:
        case NOM_FILE_OTHER: {
            if(unlink(path)) {
                nom_log(NOM_ERROR,"cannot remove file `%s`: %s", path, strerror(errno));
                return false;
            }
            nom_log(NOM_INFO, "deleted file `%s`", path);
            return true;
        }

        case NOM_FILE_DIR: {
            size_t len = strlen(path);
            char *dir = NOM_MALLOC(sizeof(*path) * (len + 1));
            NOM_ASSERT(dir != NULL && "malloc failed");
            memcpy(dir, path, len + 1);
            nom_deq_push_l(dirs, dir);
            return true;
        }

        case NOM_FILE_FAILED: {
            NOM_ASSERT(false && "unreachable");
        }
    }
    NOM_ASSERT(false && "unreachable");
}

static bool internal_nom_walkable_delete_dir_elem(const char *path, NomFileType type, NomFileStats *ftw, va_list args) {
    NomStrDeq *dirs = va_arg(args, NomStrDeq *);

    return internal_nom_delete_dir_elem(path, type, ftw, dirs);
}

bool nom_delete_dir(const char *root_dir) {
    NomStrDeq q = {0};
    bool success = nom_files_walk_tree(root_dir, internal_nom_walkable_delete_dir_elem, &q);

    while(!nom_deq_is_empty(q)) {
        char *dir = nom_deq_pop_l(&q);
        if(success) {
            if(rmdir(dir)) {
                if(errno != ENOENT) {
                    nom_log(NOM_ERROR, "cannot remove dir `%s`: %s", dir, strerror(errno));
                    success = false;
                }
            } else {
                nom_log(NOM_INFO, "deleted directory `%s`", dir);
            }
        }
        NOM_FREE(dir);
    }

    nom_deq_free(&q);

    if(success) {
        if(rmdir(root_dir)) {
            if(errno != ENOENT) {
                nom_log(NOM_ERROR, "cannot remove dir `%s`: %s", root_dir, strerror(errno));
                success = false;
            }
        } else {
            nom_log(NOM_INFO, "deleted directory `%s", root_dir);
        }
    }

    return success;
}

bool nom_delete(const char *path) {
    struct stat statbuf;
    if(stat(path, &statbuf) < 0) {
        if(errno == ENOENT) {
            return true;
        }
        nom_log(NOM_ERROR,"stat on `%s` failed: %s", path, strerror(errno));
        return false;
    }
    NomFileType file_type = internal_nom_file_type_from_stat_mode(statbuf.st_mode);

    switch(file_type) {
        case NOM_FILE_DIR: {
            return nom_delete_dir(path);
        }

        case NOM_FILE_REG:
        case NOM_FILE_OTHER: {
            if(remove(path)) {
                nom_log(NOM_ERROR,"cannot remove `%s`: %s", path, strerror(errno));
                return false;
            }
            nom_log(NOM_INFO, "deleted file `%s`", path);
            return true;
        }

        case NOM_FILE_FAILED: {
            NOM_ASSERT(false && "unreachable");
        }
    }
    NOM_ASSERT(false && "unreachable");
    return false; // Turn off gcc warning
}

bool nom_rename(const char *old_path, const char *new_path) {
    nom_log(NOM_INFO, "renaming %s -> %s", old_path, new_path);
    if(rename(old_path, new_path) < 0) {
        nom_log(NOM_ERROR, "could not rename %s to %s: %s", old_path, new_path, strerror(errno));
        return false;
    }
    return true;
}

#endif //NOM_FILES_C
