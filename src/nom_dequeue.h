#ifndef NOM_DEQUEUE_H
#define NOM_DEQUEUE_H

#include "nom_defs.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define NOM_DEQ_INIT_CAP 32

#define INTERNAL_NOM_DEQ_TMP_VAR(q) (q)->items[(q)->cap]

#define nom_deq_embed(deq_type) deq_type *items; size_t cap; size_t left; size_t right

#define NomDeq(deq_type) struct { nom_deq_embed(deq_type); }

typedef NomDeq(char *) NomStrDeq;
typedef NomDeq(const char *) NomConstStrDeq;

#define nom_deq_len(q) NOM_MOD((q).right - (q).left, (q).cap)

#define nom_deq_is_empty(q) ((q).right == (q).left)

#define nom_deq_peek_r(q) ((q).items[(q).right])

#define nom_deq_peek_l(q) ((q).items[(q).left])

#define internal_nom_deq_resize(q)                                                                                      \
    do {                                                                                                                \
        size_t left = (q)->left;                                                                                        \
        size_t old_cap = (q)->cap;                                                                                      \
        size_t new_cap = 2*old_cap;                                                                                     \
        size_t item_size = sizeof(*(q)->items);                                                                         \
                                                                                                                        \
        /* Do realloc */                                                                                                \
        void *tmp = NOM_REALLOC((q)->items, (new_cap + 1)*item_size);                                                   \
        NOM_ASSERT(tmp != NULL && "realloc failed");                                                                    \
        (q)->items = tmp;                                                                                               \
        (q)->cap = new_cap;                                                                                             \
                                                                                                                        \
        /* Sort items in new realloc memory */                                                                          \
        for(size_t new_i = old_cap, old_i = left; new_i < new_cap; new_i += 1, old_i = NOM_MOD(old_i + 1, old_cap)) {   \
            (q)->items[new_i] = (q)->items[old_i];                                                                      \
            memset(&(q)->items[old_i], 0, item_size);                                                                   \
        }                                                                                                               \
                                                                                                                        \
        /* Update indices */                                                                                            \
        (q)->left = old_cap;                                                                                            \
        memset(&(q)->right, 0, item_size);                                                                              \
    } while(0)

#define internal_nom_deq_alloc(q)                                       \
    do {                                                                \
        (q)->cap = NOM_DEQ_INIT_CAP;                                    \
        (q)->items = NOM_MALLOC(((q)->cap + 1)*sizeof(*(q)->items));    \
        NOM_ASSERT((q)->items != NULL && "malloc failed");              \
        memset((q)->items, 0, (q)->cap*sizeof(*(q)->items));            \
    } while(0)

#define nom_deq_push_r(q, e)                                    \
    do {                                                        \
        if((q)->cap == 0) {                                     \
            internal_nom_deq_alloc((q));                        \
        }                                                       \
                                                                \
        (q)->items[(q)->right] = (e);                           \
        (q)->right = NOM_MOD((q)->right + 1, (q)->cap);         \
                                                                \
        if((q)->left == (q)->right) {                           \
            /* Dequeue is full -> Resize */                     \
            internal_nom_deq_resize((q));                       \
        }                                                       \
    } while(0)

#define nom_deq_push_l(q, e)                                    \
    do {                                                        \
        if(nom_deq_is_empty(*(q))) {                            \
            nom_deq_push_r((q), (e));                           \
            break;                                              \
        }                                                       \
                                                                \
        (q)->left = NOM_MOD((q)->left - 1, (q)->cap);           \
        (q)->items[(q)->left] = (e);                            \
                                                                \
        if((q)->left == (q)->right) {                           \
            /* Dequeue is full -> Resize */                     \
            internal_nom_deq_resize((q));                       \
        }                                                       \
    } while(0)

#define nom_deq_pop_l(q) (                                                  \
        NOM_ASSERT(!nom_deq_is_empty(*(q)) && "dequeue is empty"),          \
        INTERNAL_NOM_DEQ_TMP_VAR((q)) = (q)->items[(q)->left],              \
        memset(&(q)->items[(q)->left], 0, sizeof(*(q)->items)),             \
        (q)->left = NOM_MOD((q)->left + 1, (q)->cap),                       \
        INTERNAL_NOM_DEQ_TMP_VAR((q))                                       \
    )

#define nom_deq_pop_r(q) (                                                  \
        NOM_ASSERT(!nom_deq_is_empty(*(q)) && "dequeue is empty"),          \
        (q)->right = NOM_MOD((q)->right - 1, (q)->cap),                     \
        INTERNAL_NOM_DEQ_TMP_VAR((q)) = (q)->items[(q)->right],             \
        memset(&(q)->items[(q)->right], 0, sizeof(*(q)->items)),            \
        INTERNAL_NOM_DEQ_TMP_VAR((q))                                       \
    )

#define nom_deq_free(q)                 \
    do {                                \
        if((q)->items) {                \
            NOM_FREE((q)->items);       \
        }                               \
        (q)->left = 0;                  \
        (q)->right = 0;                 \
        (q)->cap = 0;                   \
    } while(0)

#endif //NOM_DEQUEUE_H
