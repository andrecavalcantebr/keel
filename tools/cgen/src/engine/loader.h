/* engine/loader.h — the one edge that goes up, from the engine to the
 * tool (cgen-tool §3.1 [D8]). The parser needs a module's exported
 * symbols to recognize anything that uses them; finding and reading the
 * file is the tool's job. So the parser calls back through a function
 * pointer, and the engine never learns what a path is.
 *
 * What this buys, in the design's own words: "um teste entrega bytes e
 * um `KLoader` de mentira que devolve módulos de um vetor em memória...
 * nenhum diretório temporário, nenhum `mkstemp`, nenhuma limpeza."
 * engine/: no I/O, no allocation. */
#ifndef CGEN_ENGINE_LOADER_H
#define CGEN_ENGINE_LOADER_H

#include "engine/symtab.h"

typedef enum {
    K_LOAD_OK,         /* loaded now; *out is the module */
    K_LOAD_ALREADY,    /* seen before. *out set: already finished, use it.
                          *out NULL: still on the load stack, i.e. a cycle */
    K_LOAD_NOT_FOUND,  /* no root has it; the tool reports, it knows the roots */
    K_LOAD_ERROR       /* the tool already diagnosed it */
} KLoadResult;

/* What the engine needs of a loaded module. `closure_mtime` belongs to
 * the tool (spec §5's staleness rule) and is carried, never read here. */
typedef struct KModule {
    keel_slice_char name;         /* the module's own declared name */
    const KSymbol  *symbols;      /* its exported symbols, in declaration order */
    size_t          symbol_count;
    long long       closure_mtime;
} KModule;

typedef struct {
    KLoadResult (*load)(void *tool, keel_slice_char module_name, KModule **out);
    void *tool;
} KLoader;

#endif /* CGEN_ENGINE_LOADER_H */
