#ifndef NOM_SB_H
#define NOM_SB_H

#include "nom_darr.h"
#include "nom_sv.h"

typedef struct NomStringBuilder {
    nom_darr_embed(char);
} NomStringBuilder;

// Create string builder from malloced string
NomStringBuilder nom_sb_from_str(char *heap_str);

// Append a character to the string builder
void nom_sb_append_char(NomStringBuilder *sb, char c);

// Append a sized buffer to the string builder
void nom_sb_append_buf(NomStringBuilder *sb, const char *buf, size_t len);

// Append a string builder to the string builder
void nom_sb_append_sb(NomStringBuilder *sb, NomStringBuilder sb2);

// Append a NULL-terminated string to the string builder
void nom_sb_append_str(NomStringBuilder *sb, const char *str);

// Append a string view to the string builder
void nom_sb_append_sv(NomStringBuilder *sb, NomStringView sv);

// Append a single NULL character at the right of the string builder. So then you can
// use it a NULL-terminated C string
void nom_sb_append_null(NomStringBuilder *sb);

// Append newline
void nom_sb_append_nl(NomStringBuilder *sb);

// Copy String Builder
NomStringBuilder nom_sb_copy(NomStringBuilder sb);

// Reset String Builder without freeing its memory
void nom_sb_reset(NomStringBuilder *sb);

// Free the memory allocated by a string builder
// It may be reused
void nom_sb_free(NomStringBuilder *sb);

// Lsat element of string builder
char nom_sb_last(NomStringBuilder sb);

// Get string view of current string builder data
NomStringView nom_sb_to_sv(NomStringBuilder sb);

#endif //NOM_SB_H

#ifdef NOM_IMPLEMENTATION
#include "nom_sb.c"
#endif //NOM_IMPLEMENTATION
