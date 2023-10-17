#ifndef NOM_FILES_H
#define NOM_FILES_H

#include "nom_sv.h"

#include <stdarg.h>

// if POSIX
#include <sys/stat.h>

typedef enum NomFileType {
    NOM_FILE_FAILED = 0,
    NOM_FILE_REG,
    NOM_FILE_DIR,
    NOM_FILE_OTHER,
} NomFileType;

NomFileType nom_file_type(const char *path);

const char *nom_file_type_str(NomFileType file_type);

typedef struct NomFileStats {
    size_t path_len;
    size_t base_root;
    size_t base_name;
    size_t level;
    // if POSIX
    const struct stat *stat;
} NomFileStats;

bool nom_files_walk_tree(const char *root_dir, bool (*file_callback)(const char *path, NomFileType type, NomFileStats *ftw, va_list args), ...);

bool nom_files_read_dir(const char *dir, bool (*file_callback)(const char *path, NomFileType type, NomFileStats *ftw, va_list args), ...);

bool nom_mkdir(const char *path);

NomStringBuilder nom_read_file(const char *path);

NomStringBuilder nom_read_fd(int fd);

bool nom_write_file(const char *path, NomStringView data);

// Get malloced cwd string
char *nom_get_cwd(void);

bool nom_file_exists(const char *file_path);

bool nom_delete(const char *path);

bool nom_rename(const char *old_path, const char *new_path);

#endif //NOM_FILES_H

#ifdef NOM_IMPLEMENTATION
#include "nom_files.c"
#endif //NOM_IMPLEMENTATION
