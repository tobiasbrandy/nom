#ifndef NOM_SV_C
#define NOM_SV_C

#include "nom_sv.h"

#include <ctype.h>

NomStringView nom_sv_chop_by_delim(NomStringView *sv, char delim) {
    size_t i;
    for(i = 0; i < sv->len && sv->data[i] != delim; ++i);

    // Update sv to after chop
    if (i < sv->len) {
        // Skip delim
        sv->len -= i + 1;
        sv->data  += i + 1;
    } else {
        sv->len -= i;
        sv->data  += i;
    }

    // Return chop
    return nom_sv(sv->data, i);
}

NomStringView nom_sv(const char *data, size_t len) {
    NomStringView sv = {
        .len = len,
        .data = data,
    };
    return sv;
}

NomStringView nom_sv_trim_left(NomStringView sv) {
    size_t i;
    for(i = 0; i < sv.len && isspace(sv.data[i]); ++i);

    return nom_sv(sv.data + i, sv.len - i);
}

NomStringView nom_sv_trim_right(NomStringView sv) {
    size_t i;
    for(i = 0; i < sv.len && isspace(sv.data[sv.len - 1 - i]); ++i);

    return nom_sv(sv.data, sv.len - i);
}

NomStringView nom_sv_trim(NomStringView sv) {
    return nom_sv_trim_right(nom_sv_trim_left(sv));
}

NomStringView nom_sv_from_str(const char *str) {
    return nom_sv(str, strlen(str));
}

bool nom_sv_eq(NomStringView a, NomStringView b) {
    return a.len == b.len && memcmp(a.data, b.data, a.len) == 0;
}

#endif //NOM_SV_C
