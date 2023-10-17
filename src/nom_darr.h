#ifndef NOM_DARR_H
#define NOM_DARR_H

#include "nom_defs.h"

#include <string.h>

// Initial capacity of a dynamic array
#define NOM_DARR_INIT_CAP 32

// Embeds in struct dynamic array of type darr_type
#define nom_darr_embed(darr_type) darr_type *items; size_t len; size_t cap

// Declare dynamic array of type darr_type
#define NomDarr(darr_type) struct { nom_darr_embed(darr_type); }

typedef NomDarr(char *) NomStrDarr;
typedef NomDarr(const char *) NomConstStrDarr;

// Append an item to a dynamic array
#define nom_darr_append(darr, item)                                                                 \
    do {                                                                                            \
        if ((darr)->len >= (darr)->cap) {                                                           \
            (darr)->cap = (darr)->cap == 0 ? NOM_DARR_INIT_CAP : (darr)->cap*2;                     \
            void *tmp = NOM_REALLOC((darr)->items, (darr)->cap*sizeof(*(darr)->items));             \
            NOM_ASSERT(tmp != NULL && "realloc failed");                                            \
            (darr)->items = tmp;                                                                    \
        }                                                                                           \
                                                                                                    \
        (darr)->items[(darr)->len++] = (item);                                                      \
    } while (0)

// Reset dynamic array without freeing its memory
#define nom_darr_reset(da) (da)->len = 0

// Set dynamic array to it's zero value
#define nom_darr_free(da)           \
    do {                            \
        if((da)->items) {           \
            NOM_FREE((da)->items);  \
            (da)->items = NULL;     \
        }                           \
        (da)->len = 0;              \
        (da)->cap = 0;              \
    } while(0)

// Append several items to dynamic array
#define nom_darr_append_many(darr, new_items, new_items_len)                                    \
    do {                                                                                        \
        if ((darr)->len + new_items_len > (darr)->cap) {                                        \
            if ((darr)->cap == 0) {                                                             \
                (darr)->cap = NOM_DARR_INIT_CAP;                                                \
            }                                                                                   \
            while ((darr)->len + new_items_len > (darr)->cap) {                                 \
                (darr)->cap *= 2;                                                               \
            }                                                                                   \
            void *tmp = NOM_REALLOC((darr)->items, (darr)->cap*sizeof(*(darr)->items));         \
            NOM_ASSERT(tmp != NULL && "realloc failed");                                        \
           (darr)->items = tmp;                                                                 \
        }                                                                                       \
        memcpy((darr)->items + (darr)->len, new_items, new_items_len*sizeof(*(darr)->items));   \
        (darr)->len += new_items_len;                                                           \
    } while (0)

// Iterate dynamic array
#define nom_darr_foreach(iter, darr)                                    \
    if((darr).len > 0) iter = (darr).items[0];                          \
    for(size_t i = 0; i < (darr).len; i += 1, iter = (darr).items[i])

#endif //NOM_DARR_H
