#ifndef NOM_LOG_H
#define NOM_LOG_H

#include <stdio.h>

typedef enum {
    NOM_INFO = 0,
    NOM_WARNING,
    NOM_ERROR,
} NomLogLevel;

void nom_log_set_level(NomLogLevel level);

void nom_log_set_stream(FILE *stream);

void nom_log(NomLogLevel level, const char *fmt, ...);

#endif //NOM_LOG_H

#ifdef NOM_IMPLEMENTATION
#include "nom_log.c"
#endif //NOM_IMPLEMENTATION
