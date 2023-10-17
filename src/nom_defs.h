#ifndef NOM_DEFS_H
#define NOM_DEFS_H

#include <stdint.h>

#ifndef NOM_ASSERT
    #include <assert.h>
    #define NOM_ASSERT assert
#endif //NOM_ASSERT

#if defined(NOM_MALLOC) && defined(NOM_REALLOC) && defined(NOM_FREE)
    // ok
#elif !defined(NOM_MALLOC) && !defined(NOM_REALLOC) && !defined(NOM_FREE)
    #include <stdlib.h>
    #define NOM_MALLOC malloc
    #define NOM_REALLOC realloc
    #define NOM_FREE free
#else
    #error "Must define all or none of NOM_MALLOC, NOM_REALLOC and NOM_FREE."
#endif

#ifndef NOM_REBUILD_YOURSELF_CC
    #define NOM_REBUILD_YOURSELF_CC "cc"
#endif

#ifndef NOM_REBUILD_YOURSELF_FLAGS
    #define NOM_REBUILD_YOURSELF_FLAGS "-Wall", "-Wextra", "-pedantic", "-Wshadow", "-Wformat=2", "-Wno-unused-parameter", "-Wno-unused-function", "-Wno-implicit-fallthrough"
#endif

#define NOM_FREE_CONST(ptr) NOM_FREE((void *)(uintptr_t)ptr)

#define NOM_ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))

#define nom_return_defer(value) do { ret = (value); goto defer; } while(0)

#define NOM_MOD(i, n) (((i) % (n) + (n)) % (n))

#endif //NOM_DEFS_H
