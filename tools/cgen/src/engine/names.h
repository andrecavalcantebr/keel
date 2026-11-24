#ifndef CGEN_NAMES_H
#define CGEN_NAMES_H
#include "engine/loader.h"
/* Compact a qualified name; SIZE_MAX means malformed or insufficient space. */
size_t k_name_normalize(keel_slice_char, keel_slice_char);
/* Resolve qualified names through module identities, never through headers. */
const KSymbol *k_symbol_resolve(const KSymbolTable *, keel_slice_char);
const char *k_primitive_name(keel_slice_char);
bool k_c_arith_word(KToken);
const char *k_c_words_primitive(const KToken *,size_t);
size_t k_mangle_symbol(const KSymbol *, keel_slice_char);
#endif
