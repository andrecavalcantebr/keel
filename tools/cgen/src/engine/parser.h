/* engine/parser.h — the parser's first recognizers, built directly on top of
 * the lexer (parser-design §3.2, etapa 1: header). No KParserCtx yet — this
 * header grows one production at a time, the same way lexer.h grew through
 * M1's recognizers. */
#ifndef CGEN_ENGINE_PARSER_H
#define CGEN_ENGINE_PARSER_H

#include "engine/lexer.h"
#include "engine/symtab.h"

/* Scans a qualified-name (keel-spec §2.2: `qualified-name ::= IDENT { '.'
 * IDENT }`), the production shared by `module-name`, the module path of
 * `import`, and every dotted type or verb reference. `first` must already be
 * an IDENT token. Returns the slice from `first`'s first byte to the last
 * IDENT's last byte (trivia in between is part of the slice, not stripped).
 * Consumes exactly one token past the name and hands it back through
 * next_out/next_pp_kind_out — already read from `lexer`, so the caller uses
 * it instead of calling k_lexer_next again for it. Assumes well-formed
 * input: a `.` here is always followed by another IDENT. */
keel_slice_char k_scan_qualified_name(KLexer *lexer, KToken first,
                                       KToken *next_out, TKPpKind *next_pp_kind_out);

/* Scans `IDENT { ',' IDENT }` — the shape of keel-spec §2.2's dim-binder,
 * tags-binder and type-binder, after their own keyword ('dim'/'tags'/
 * 'type') has already been consumed. `first` is already the first IDENT.
 * Stores each IDENT token into out[0..*count_out), up to cap entries.
 * Returns false, without writing past out[cap-1], if the list has more
 * than cap entries (never happens in well-formed keel source, but must not
 * corrupt memory if it somehow did). Consumes one token past the list and
 * hands it back via next_out/next_pp_kind_out, same convention as every
 * other recognizer in this file. */
bool k_scan_ident_list(KLexer *lexer, KToken first, KToken *out, size_t cap,
                        size_t *count_out, KToken *next_out, TKPpKind *next_pp_kind_out);

/* decl-module (keel-spec §2.2):
 *   'module' module-name [dim-binder] [tags-binder] [type-binder] ';'
 * `module_kw` is the already-consumed 'module' token. The three binders
 * are each optional and, when present, appear in this exact order.
 * Consumes through the terminating ';' and hands back the token after it,
 * same lookahead convention as k_scan_qualified_name. */
typedef struct {
    keel_slice_char module_name;
    KToken dims[8];  size_t dim_count;
    KToken tags[8];  size_t tag_count;
    KToken types[8]; size_t type_count;
} KModuleHeader;

void k_scan_module_decl(KLexer *lexer, KToken module_kw, KModuleHeader *out,
                         KToken *next_out, TKPpKind *next_pp_kind_out);

/* import (keel-spec §2.2): 'import' module-name ['as' IDENT] ['types'] ';'
 * `import_kw` is the already-consumed 'import' token. `as` and `types` are
 * each optional and, when present, appear in this order. `alias.len == 0`
 * means no `as` clause was written. Consumes through ';' and hands back the
 * token after it. */
typedef struct {
    keel_slice_char module_name;
    keel_slice_char alias;
    bool has_types;
} KImportDecl;

void k_scan_import(KLexer *lexer, KToken import_kw, KImportDecl *out,
                    KToken *next_out, TKPpKind *next_pp_kind_out);

/* import_c (keel-spec §2.2): 'import_c' (system-header | STRING) ';'
 * `import_c_kw` is the already-consumed 'import_c' token. `header` is the
 * raw text of whichever form was written, delimiters included:
 * `<stdio.h>` or `"foo.h"`. Consumes through ';' and hands back the token
 * after it. */
typedef struct { keel_slice_char header; } KImportCDecl;

void k_scan_import_c(KLexer *lexer, KToken import_c_kw, KImportCDecl *out,
                      KToken *next_out, TKPpKind *next_pp_kind_out);

/* Scans a balanced '{' ... '}' region — the shape behind extern-c's body
 * (below), and later a modifier's body (§4.3), a struct's body, and any
 * block. `open` is the already-consumed '{' token. Only '{' and '}' tokens
 * are counted: C guarantees any '(' ')' '[' ']' inside are themselves
 * balanced regardless, and the lexer already keeps string/char literals,
 * comments and directives from producing spurious brace tokens. Depth
 * starts at 1 for `open`; the function stops the instant it reads the '}'
 * that brings depth back to 0 — that closing '}' is consumed but is NOT
 * part of the returned slice. Returns the slice of everything strictly
 * between '{' and '}', trivia included, not stripped; an empty body
 * (`{}`) returns `{ .ptr = open.ptr + open.len, .len = 0 }` — lexer-design
 * §7: "região vazia guarda seu ponteiro de fronteira." Hands back the
 * token right after the closing '}' via next_out/next_pp_kind_out, same
 * convention as every other recognizer in this file. */
keel_slice_char k_scan_braced_opaque(KLexer *lexer, KToken open,
                                      KToken *next_out, TKPpKind *next_pp_kind_out);

/* extern-c (keel-spec §2.2): [ 'pub' | 'priv' ] 'extern_c' [ '[' 'type_h' ']' ] '{' <opaque> '}'
 * `extern_c_kw` is the already-consumed 'extern_c' token — any 'pub'/'priv'
 * before it was already consumed by the caller too, same convention as
 * `module_kw`/`import_kw`/`import_c_kw` above; this function does not read
 * either. `body` is exactly what `k_scan_braced_opaque` returns. Consumes
 * through the closing '}' and hands back the token after it. */
typedef struct {
    bool has_type_h;
    keel_slice_char body;
} KExternCDecl;

void k_scan_extern_c(KLexer *lexer, KToken extern_c_kw, KExternCDecl *out,
                      KToken *next_out, TKPpKind *next_pp_kind_out);

/* decl-modifier (keel-spec §2.2): 'modifier' IDENT ['byref'] '{' <opaque> '}'
 * — the first recognizer of etapa 2 ("coleta", parser-design §3.2): it
 * registers a symbol, not just recognizes syntax. `modifier_kw` is the
 * already-consumed 'modifier' token (any 'pub'/'priv' before it, too,
 * already consumed by the caller — same convention as every `..._kw`
 * above). `module_arity` is the sum of the declaring module's own
 * dim/tags/type binder counts (`KModuleHeader.dim_count + tag_count +
 * type_count` from k_scan_module_decl) — every modifier of a module
 * shares that one arity (keel-spec §4.3: "a assinatura do módulo fixa a
 * quantidade... de todos os seus modificadores"), so this function does
 * not need to look inside the modifier's own body to know it.
 *
 * Registers `out->name` into `symtab` as K_SYM_MODIFIER with that arity,
 * and returns what k_symtab_insert returns (false only if the table is
 * full — this function still finishes scanning the declaration either
 * way, so the caller's next_out stays correct even on that failure).
 * `out->body` is exactly what k_scan_braced_opaque returns for the
 * modifier's body — kept for a later pass, not interpreted here. Consumes
 * through the closing '}' and hands back the token after it. */
typedef struct {
    keel_slice_char name;
    bool byref;
    keel_slice_char body;
} KModifierDecl;

bool k_scan_modifier_decl(KLexer *lexer, KToken modifier_kw, int module_arity,
                           KSymbolTable *symtab, KModifierDecl *out,
                           KToken *next_out, TKPpKind *next_pp_kind_out);

/* decl-tags (keel-spec §2.2), without tag values for now:
 *   'tags' IDENT '[' IDENT { ',' IDENT } ']' ';'
 * (the full production also allows `IDENT '=' tag-value` per item —
 * deferred to a later task, same way k_scan_module_decl's binders were
 * added after its first version). `tags_kw` is the already-consumed
 * 'tags' token (any 'pub'/'priv' before it too — same convention as every
 * `..._kw` above). Registers `out->name` into `symtab` as K_SYM_TAGS with
 * arity 0 (unused for this kind), and returns what k_symtab_insert
 * returns. Consumes through the closing ';' and hands back the token
 * after it. */
typedef struct {
    keel_slice_char name;
    KToken items[16];
    size_t item_count;
} KTagsDecl;

bool k_scan_tags_decl(KLexer *lexer, KToken tags_kw, KSymbolTable *symtab,
                       KTagsDecl *out, KToken *next_out, TKPpKind *next_pp_kind_out);

/* decl-struct (keel-spec §2.2), with its fields left opaque for now (a
 * later task interprets keel fields inside — spec §4.2: "struct-spec
 * registra os campos keel... os demais campos são opacos"):
 *   struct-spec ';'
 *   struct-spec ::= ( 'struct' | 'union' ) [ IDENT ] '{' { field } '}'
 * `struct_or_union_kw` is the already-consumed 'struct'/'union' token (any
 * 'pub'/'priv' before it too — given to you, unused otherwise). `is_union`
 * is not computed by this function's caller: derive it from
 * `struct_or_union_kw`'s own spelling with
 * `k_token_is_c_word_named(struct_or_union_kw, "union")`.
 *
 * `out->tag_name.len == 0` when the struct/union is anonymous (no IDENT
 * before '{') — only register into `symtab` (as K_SYM_TYPE) when a tag
 * name exists; an anonymous struct has nothing to register, and that is
 * not a failure (return true). Unlike every recognizer above,
 * struct-spec's own grammar has no ';' right after its '}' — but
 * decl-struct's does, so k_scan_braced_opaque's next_out (the token right
 * after '}') is already sitting on that ';', not past it: one more
 * k_lexer_next call, after the braced_opaque call, is what lands
 * next_out past it. */
typedef struct {
    keel_slice_char tag_name;
    bool is_union;
    keel_slice_char body;
} KStructDecl;

bool k_scan_struct_decl(KLexer *lexer, KToken struct_or_union_kw, KSymbolTable *symtab,
                         KStructDecl *out, KToken *next_out, TKPpKind *next_pp_kind_out);

#endif /* CGEN_ENGINE_PARSER_H */
