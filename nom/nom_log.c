#ifndef NOM_LOG_C
#define NOM_LOG_C

#include "nom_log.h"

#include "nom_defs.h"

#include <stdarg.h>
#include <stdbool.h>

static NomLogLevel internal_nom_log_level = NOM_INFO;
static FILE *internal_nom_log_stream = NULL;

void nom_log_set_level(NomLogLevel level) {
    internal_nom_log_level = level;
}

void nom_log_set_stream(FILE *stream) {
    internal_nom_log_stream = stream;
}

void nom_log(NomLogLevel level, const char *fmt, ...) {
    if(level < internal_nom_log_level) {
        return;
    }

    FILE *stream = internal_nom_log_stream ? internal_nom_log_stream : stderr;

    switch(level) {
        case NOM_INFO:
            fprintf(stream, "[INFO] ");
            break;
        case NOM_WARNING:
            fprintf(stream, "[WARNING] ");
            break;
        case NOM_ERROR:
            fprintf(stream, "[ERROR] ");
            break;
        default:
            NOM_ASSERT(false && "unreachable");
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stream, fmt, args);
    va_end(args);
    fprintf(stream, "\n");
}

#endif //NOM_LOG_C
