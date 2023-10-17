#ifndef NOM_SV_H
#define NOM_SV_H

typedef struct NomStringView {
    size_t len;
    const char *data;
} NomStringView;

// USAGE: printf("SV: "NOM_SV_FMT"\n", NOM_SV_ARG(sv));
#define NOM_SV_FMT "%.*s"
#define NOM_SV_ARG(sv) (int) (sv).count, (sv).data

NomStringView nom_sv_chop_by_delim(NomStringView *sv, char delim);

NomStringView nom_sv_trim_left(NomStringView sv);
NomStringView nom_sv_trim_right(NomStringView sv);
NomStringView nom_sv_trim(NomStringView sv);

bool nom_sv_eq(NomStringView a, NomStringView b);

NomStringView nom_sv_from_str(const char *str);

NomStringView nom_sv(const char *data, size_t len);

#endif //NOM_SV_H

#ifdef NOM_IMPLEMENTATION
#include "nom_sv.c"
#endif //NOM_IMPLEMENTATION
