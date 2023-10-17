#ifndef NOM_CMD_H
#define NOM_CMD_H

#include "nom_sb.h"

#include <stdbool.h>

#include <sys/types.h>

typedef pid_t NomProc;

#define NOM_INVALID_PROC (-1)

typedef NomDarr(NomProc) NomProcs;

typedef struct NomCmd {
    int out_fd;
    const char *out_path;
    nom_darr_embed(char *);
} NomCmd;

typedef NomDarr(const char *) NomCmdFlags;

// Append flags
#define nom_cmd_flags_append(flags, ...) \
    nom_cmd_flags_append_buf(flags, ((const char*[]){__VA_ARGS__}), (sizeof((const char*[]){__VA_ARGS__})/sizeof(const char*)))

// Append flags
void nom_cmd_flags_append_buf(NomCmdFlags *flags, const char *buf[], size_t len);

// Append command arguments
#define nom_cmd_append(cmd, ...) \
    nom_cmd_append_buf(cmd, ((const char*[]){__VA_ARGS__}), (sizeof((const char*[]){__VA_ARGS__})/sizeof(const char*)))

// Run command inline
#define nom_cmd_run(cmd, ...) \
    nom_cmd_run_buf(cmd, ((const char*[]){__VA_ARGS__}), (sizeof((const char*[]){__VA_ARGS__})/sizeof(const char*)))

// Append command arguments
void nom_cmd_append_buf(NomCmd *cmd, const char *buf[], size_t len);

// Render a string representation of a command into a string builder. Keep in mind the the
// string builder is not NULL-terminated by default. Use nob_sb_append_null if you plan to
// use it as a C string.
void nom_cmd_render(NomCmd cmd, NomStringBuilder *render);

void nom_cmd_append_cmd(NomCmd *cmd, NomCmd cmd2);

void nom_cmd_append_flags(NomCmd *cmd, NomCmdFlags flags);

// Reset command without freeing its memory
void nom_cmd_reset(NomCmd *cmd);

// Free all the memory allocated by command arguments
void nom_cmd_free(NomCmd *cmd);

// Run command asynchronously
NomProc nom_cmd_run_async(NomCmd cmd);

// Wait for process
bool nom_proc_wait(NomProc proc);

// Wait for multiple processes
bool nom_procs_wait(NomProcs procs);

// Run command synchronously
bool nom_cmd_run_sync(NomCmd cmd);

bool nom_cmd_run_buf(NomCmd *cmd, const char *buf[], size_t len);

#endif //NOM_CMD_H

#ifdef NOM_IMPLEMENTATION
#include "nom_cmd.c"
#endif //NOM_IMPLEMENTATION
