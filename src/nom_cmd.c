#ifndef NOM_CMD_C
#define NOM_CMD_C

#include "nom_darr.h"

#include "nom_log.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>

void nom_cmd_flags_append_buf(NomCmdFlags *flags, const char *buf[], size_t len) {
    nom_darr_append_many(flags, buf, len);
}

void nom_cmd_append_buf(NomCmd *cmd, const char *buf[], size_t len) {
    nom_darr_append_many(cmd, buf, len);
}

void nom_cmd_append_cmd(NomCmd *cmd, NomCmd cmd2) {
    nom_darr_append_many(cmd, cmd2.items, cmd2.len);
}

void nom_cmd_append_flags(NomCmd *cmd, NomCmdFlags flags) {
    nom_darr_append_many(cmd, flags.items, flags.len);
}

void nom_cmd_reset(NomCmd *cmd) {
    cmd->out_fd = 0;
    cmd->out_path = NULL;
    nom_darr_reset(cmd);
}

void nom_cmd_free(NomCmd *cmd) {
    nom_darr_free(cmd);
}

void nom_cmd_render(NomCmd cmd, NomStringBuilder *render) {
    for(size_t i = 0; i < cmd.len; ++i) {
        const char *arg = cmd.items[i];
        if(arg == NULL){
            break;
        }
        if(i > 0){
            nom_sb_append_char(render, ' ');
        }
        if(!strchr(arg, ' ')) {
            nom_sb_append_str(render, arg);
        } else {
            nom_sb_append_char(render, '\'');
            nom_sb_append_str(render, arg);
            nom_sb_append_char(render, '\'');
        }
    }
}

NomProc nom_cmd_run_async(NomCmd cmd) {
    NOM_ASSERT((!cmd.out_fd || !cmd.out_path) && "cannot set both output fd and path");

    if(cmd.len < 1) {
        nom_log(NOM_ERROR, "Could not run empty command");
        return NOM_INVALID_PROC;
    }

    NomStringBuilder sb = {0};
    nom_cmd_render(cmd, &sb);
    nom_sb_append_null(&sb);
    nom_log(NOM_INFO, "CMD: %s", sb.items);
    nom_sb_free(&sb);

    pid_t child_pid = fork();
    if(child_pid < 0) {
        nom_log(NOM_ERROR, "Could not fork child process: %s", strerror(errno));
        return NOM_INVALID_PROC;
    }

    if(child_pid == 0) { // Child
        int out_fd = cmd.out_fd;
        if(cmd.out_path) {
            static const int flags = O_WRONLY | O_CREAT | O_TRUNC;
            static const mode_t mode =
                S_IRWXU                 // Owner: Read, Write, Execute
                | S_IRGRP | S_IXGRP     // Group: Read, Execute
                | S_IROTH | S_IXOTH     // Others: Read, Execute
                ;
            if((out_fd = open(cmd.out_path, flags, mode)) == -1) {
                nom_log(NOM_ERROR, "Could not open `%s` for command output: %s", cmd.out_path, strerror(errno));
                return NOM_INVALID_PROC;
            }
        }
        if(out_fd != STDIN_FILENO && out_fd != STDOUT_FILENO) {
            if(dup2(out_fd, STDOUT_FILENO) == -1) {
                nom_log(NOM_ERROR, "Could not set fd `%d` for command output: %s", out_fd, strerror(errno));
                return NOM_INVALID_PROC;
            }
            close(out_fd);
        }

        nom_cmd_append(&cmd, NULL);
        if(execvp(cmd.items[0], cmd.items) < 0) {
            nom_log(NOM_ERROR, "Could not exec child process: %s", strerror(errno));
            exit(1);
        }
        NOM_ASSERT(false && "unreachable");
    }

    return child_pid;
}

bool nom_proc_wait(NomProc proc) {
    if(proc == NOM_INVALID_PROC) {
        return false;
    }

    while(true) {
        int wait_status = 0;
        if(waitpid(proc, &wait_status, 0) < 0) {
            nom_log(NOM_ERROR, "could not wait on command (pid %d): %s", proc, strerror(errno));
            return false;
        }

        if(WIFEXITED(wait_status)) {
            int exit_status = WEXITSTATUS(wait_status);
            if(exit_status != 0) {
                nom_log(NOM_ERROR, "command exited with exit code %d", exit_status);
                return false;
            }
            break;
        }

        if(WIFSIGNALED(wait_status)) {
            nom_log(NOM_ERROR, "command process was terminated by %s", strsignal(WTERMSIG(wait_status)));
            return false;
        }
    }

    return true;
}

bool nom_procs_wait(NomProcs procs) {
    bool success = true;
    for(size_t i = 0; i < procs.len; ++i) {
        success = nom_proc_wait(procs.items[i]) && success;
    }
    return success;
}

bool nom_cmd_run_sync(NomCmd cmd) {
    NomProc p = nom_cmd_run_async(cmd);
    return nom_proc_wait(p);
}

bool nom_cmd_run_buf(NomCmd *cmd, const char *buf[], size_t len) {
    nom_cmd_append_buf(cmd, buf, len);
    bool ret = nom_cmd_run_sync(*cmd);
    nom_cmd_reset(cmd);
    return ret;
}

#endif //NOM_CMD_C
