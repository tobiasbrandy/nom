#ifndef NOM_COMPILE_H
#define NOM_COMPILE_H

typedef struct NomCompileConfig {
    const char *cc;
    const char *target;
    const char *src_dir;
    const char *obj_dir;
    NomCmdFlags flags;
} NomCompileConfig;

bool nom_needs_rebuild(const char *target_path, const char * const dependencies[], size_t dependencies_count);

//   How to use it:
//     int main(int argc, const char** argv) {
//         nom_rebuild_yourself(argc, argv, __FILE__);
//         // actual work
//         return 0;
//     }
//
//   After your added this macro every time you run ./build it will detect
//   that you modified its original source code and will try to rebuild itself
//   before doing any actual work. So you only need to bootstrap your build system
//   once.
//
//   The modification is detected by comparing the last modified times of the executable
//   and its source code. The same way the make utility usually does it.
//
void nom_rebuild_yourself(int argc, const char **argv, const char *src_path);

bool nom_compile(const NomCompileConfig *config);

bool nom_build_compilation_database(const NomCompileConfig *config);

bool nom_clean(const NomCompileConfig *compile_config);

#endif //NOM_COMPILE_H

#ifdef NOM_IMPLEMENTATION
#include "nom_compile.c"
#endif //NOM_IMPLEMENTATION
