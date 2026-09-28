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

/* The load state lives in the tool (cgen-tool §3.1): per module, one of
 * never seen / pending / done / failed. Which of those a request lands
 * on is what picks the result below — and only a request for a PENDING
 * module is a cycle. A request for a module that is done is a
 * memoization hit even while an ancestor further up the stack is still
 * pending. "Pending" is therefore a state, never a result: a pending
 * module is one whose load is in progress at another level of the
 * stack, and asking for it again is exactly the cycle. */
typedef enum {
    K_LOAD_OK,         /* loaded now, and complete */
    K_LOAD_ALREADY,    /* was already loaded and complete: the memoized one */
    K_LOAD_CYCLE,      /* a request for a pending module: closes a cycle */
    K_LOAD_NOT_FOUND,  /* resolver reports at the import; root caller reports at CLI */
    K_LOAD_ERROR       /* the tool already diagnosed it */
} KLoadResult;

/* What the engine needs of a loaded module. `closure_mtime` belongs to
 * the tool (spec §5's staleness rule) and is carried, never read here. */
typedef struct KModule {
    keel_slice_char name;         /* the module's own declared name */
    const KSymbol  *symbols;      /* its exported symbols, in declaration order */
    size_t          symbol_count;
    long long       closure_mtime;
    const struct KAst *ast; /* retained source-backed tree */
} KModule;

typedef struct {
    KLoadResult (*load)(void *tool, keel_slice_char module_name, KModule **out);
    void *tool;
} KLoader;

#endif /* CGEN_ENGINE_LOADER_H */
