#ifndef NOM_SB_C
#define NOM_SB_C

#include "nom_sb.h"

#include <string.h>

NomStringBuilder nom_sb_from_str(char *heap_str) {
    size_t len = strlen(heap_str) + 1;
    NomStringBuilder ret = {
        .items = heap_str,
        .len = len,
        .cap = len,
    };
    return ret;
}

void nom_sb_append_char(NomStringBuilder *sb, char c) {
    nom_darr_append(sb, c);
}

void nom_sb_append_buf(NomStringBuilder *sb, const char *buf, size_t len) {
    nom_darr_append_many(sb, buf, len);
}

void nom_sb_append_sb(NomStringBuilder *sb, NomStringBuilder sb2) {
    nom_darr_append_many(sb, sb2.items, sb2.len);
}

void nom_sb_append_str(NomStringBuilder *sb, const char *str) {
    size_t n = strlen(str);
    nom_darr_append_many(sb, str, n);
}

void nom_sb_append_sv(NomStringBuilder *sb, NomStringView sv) {
    nom_darr_append_many(sb, sv.data, sv.len);
}

void nom_sb_append_null(NomStringBuilder *sb) {
    nom_darr_append(sb, 0);
}

void nom_sb_append_nl(NomStringBuilder *sb) {
    nom_darr_append(sb, '\n');
}

inline NomStringBuilder nom_sb_copy(NomStringBuilder sb) {
    NomStringBuilder ret = {0};
    nom_sb_append_sb(&ret, sb);
    return ret;
}

void nom_sb_reset(NomStringBuilder *sb) {
    nom_darr_reset(sb);
}

void nom_sb_free(NomStringBuilder *sb) {
    nom_darr_free(sb);
}

inline char nom_sb_last(NomStringBuilder sb) {
    NOM_ASSERT(sb.len > 0 && "String Builder is empty");
    return sb.items[sb.len-1];
}

inline NomStringView nom_sb_to_sv(NomStringBuilder sb) {
    return nom_sv(sb.items, sb.len);
}

#endif //NOM_SB_C
