#define NOB_IMPLEMENTATION
#include "nom.h"

#include <string.h>

void set_flags(NomCompileConfig *compile_config) {
    NomCmdFlags flags = {0};

    nom_cmd_flags_append(&flags // INCLUDES
        , "-iquote"
        , compile_config->src_dir
    );
    nom_cmd_flags_append(&flags // CFLAGS
        , "-Wall"
        , "-Wextra"
        , "-pedantic"
        , "-Wshadow"
        , "-Wformat=2"
    );
    nom_cmd_flags_append(&flags // OFF_WARNINGS
        , "-Wno-unused-parameter"
        , "-Wno-unused-function"
        , "-Wno-implicit-fallthrough"
    );
    nom_cmd_flags_append(&flags // DEBUG_FLAGS
        , "-O0"
        , "-ggdb3"
        , "-fsanitize=undefined"
        , "-fno-omit-frame-pointer"
        , "-fstack-protector"
    );
    nom_cmd_flags_append(&flags // GDB_INCOMPATIBLE_FLAGS
        , "-fsanitize=address,pointer-compare,pointer-subtract,leak"
    );

    compile_config->flags = flags;
}

#define DEFAULT_CMD "compile"
int main(int argc, const char **argv) {
    nom_rebuild_yourself(argc, argv, __FILE__);

    const char *cmd = argc > 1 ? argv[1] : DEFAULT_CMD;

    NomCompileConfig compile_config = {
            .cc         = "gcc",
            .target     = "a.out",
            .src_dir    = "src",
            .obj_dir    = "obj",
    };
    set_flags(&compile_config);

    int ret = 0;

    if(strcmp(cmd, "compile") == 0) {
        if(!nom_compile(&compile_config)) ret = 1;

    } else if(strcmp(cmd, "db") == 0) {
        if(!nom_build_compilation_database(&compile_config)) ret = 1;

    } else if(strcmp(cmd, "clean") == 0) {
        if(!nom_clean(&compile_config)) ret = 1;

    } else {
        nom_log(NOM_ERROR, "command `%s` not recognized", cmd);
        ret = 1;
    }

    nom_darr_free(&compile_config.flags);

    return ret;
}
