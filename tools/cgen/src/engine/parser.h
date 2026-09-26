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

/* The first piece of keel-spec §2.2's `specifier` / parser-design §4's
 * dispatch — steps 2-3 only ("IDENT registrado como modificador? ... IDENT
 * registrado como tipo?"), single-token arguments only (no nested
 * known-type, no qualifiers — `buffer i32`, not `buffer const T`, and not
 * a modifier applied to another modifier's result). `first` is already
 * read (an IDENT) — this function does not read anything beyond it unless
 * it recognizes a specifier.
 *
 * `k_symtab_lookup(symtab, first)`'s result alone decides which of the
 * three outcomes applies — no more than one token of lookahead is ever
 * needed beyond `first` itself:
 *
 *  - Not found in `symtab` at all: `out->kind = K_SPEC_NONE`. This
 *    function has not consumed anything beyond `first` — do not read
 *    `*next_out`, it is left untouched; the caller treats `first` itself
 *    as the start of opaque C, exactly where the lexer already is.
 *  - Found as `K_SYM_MODIFIER`: `out->kind = K_SPEC_MODIFIER`,
 *    `out->modifier_name = first`. Reads exactly `sym->arity` further
 *    tokens, one per argument (no recursion — each argument is kept as
 *    its own single token, into `out->args[0..arity)`), then one more
 *    read into `*next_out` (the declarator's own start). Returns false,
 *    without reading past the 4th argument, if `arity` is more than 4 —
 *    real keel modules registered so far never need that many.
 *  - Found as `K_SYM_TYPE`: `out->kind = K_SPEC_NAMED_TYPE`,
 *    `out->type_name = first`. Reads exactly one more token into
 *    `*next_out`.
 *
 * A symbol of any other `KSymKind` (found, but neither of those two) is
 * treated the same as not found: `K_SPEC_NONE`. */
typedef enum { K_SPEC_NONE, K_SPEC_MODIFIER, K_SPEC_NAMED_TYPE } KSpecifierKind;

typedef struct {
    KSpecifierKind kind;
    keel_slice_char modifier_name;   /* meaningful when kind == K_SPEC_MODIFIER */
    KToken args[4];  size_t arg_count;
    keel_slice_char type_name;       /* meaningful when kind == K_SPEC_NAMED_TYPE */
} KSpecifier;

bool k_scan_known_type(KLexer *lexer, KToken first, const KSymbolTable *symtab,
                        KSpecifier *out, KToken *next_out, TKPpKind *next_pp_kind_out);

/* The common case of keel-spec §2.2's `declarator`:
 *   declarator ::= { '*' { qual-c } } direct-declarator
 *   direct-declarator ::= IDENT { suffix }
 * — just `{ '*' } IDENT`, no `qual-c` between stars (`const`, `ref`, ...)
 * and no trailing `suffix` (array brackets, function parameter groups)
 * yet; both are later tasks. `first` is already read — it is either the
 * declarator's leading `'*'`, or, if there is none, the `IDENT` itself.
 * Counts every leading `'*'` into `out->pointer_depth`, then the next
 * token is the name. Consumes one token past the name and hands it back
 * via next_out/next_pp_kind_out, same convention as every recognizer in
 * this file. */
typedef struct {
    int pointer_depth;
    keel_slice_char name;
} KDeclaratorHead;

bool k_scan_declarator_head(KLexer *lexer, KToken first, KDeclaratorHead *out,
                             KToken *next_out, TKPpKind *next_pp_kind_out);

/* A reusable utility, not tied to one grammar production: skips tokens,
 * counting `'(' '[' '{'` as +1 and `')' ']' '}'` as -1 against a single
 * shared depth (keel-spec's own delimiters, §2.1 — C guarantees they
 * nest consistently regardless of which kind opened, so one counter is
 * enough), until it reads a token that both (a) one of `terminators`
 * matches (compared with k_token_is_punct) and (b) depth is 0 at that
 * point. `first` is already read, and may itself already be a match
 * (an empty region). Sets `*terminator_index_out` to which entry of
 * `terminators` matched, and `*next_out` to that terminator token itself
 * — like every other recognizer, the terminator has already been read
 * from `lexer`, so the caller uses it instead of reading it again.
 *
 * EOF safety (tools/harness/README.md, "um oráculo tem que sobreviver a
 * um travamento"): if `first`, or any token read afterward, is empty
 * (`.len == 0`, meaning end of file), stop immediately as if it matched
 * `terminators[0]` — well-formed input never actually reaches this, but
 * looping on EOF forever is exactly the kind of hang a wrong recognizer
 * has produced before. */
void k_scan_opaque_until(KLexer *lexer, KToken first, const char *const *terminators,
                          size_t terminator_count, size_t *terminator_index_out,
                          KToken *next_out, TKPpKind *next_pp_kind_out);

/* A first decl-keel (keel-spec §2.2):
 *   specifier init-decl { ',' init-decl } ';'
 *   init-decl ::= declarator [ '=' <opaque> ]
 * — the common case: no `spec-c` prefix (`inline`/`static`/...), no
 * `array`/`constexpr` alternative forms, no `else`-tail — all later
 * tasks. `first` is already read (the specifier's own first token).
 *
 * Calls `k_scan_known_type` first: if it comes back `K_SPEC_NONE`, this
 * function is not applicable either — return `false` immediately,
 * without reading anything beyond `first` and without touching
 * `*next_out`, exactly like `k_scan_known_type` itself does in that case.
 * Otherwise, for each `init-decl` (there is always at least one):
 * `k_scan_declarator_head` gets the name; register it into `symtab` as
 * `K_SYM_VARIABLE` (arity 0); if an `'='` follows, skip the initializer
 * with `k_scan_opaque_until(lexer, ..., (const char *[]){ ",", ";" }, 2,
 * ...)`; a `','` means another `init-decl` follows, a `';'` ends the
 * declaration. Consumes through that `';'` and hands back the token
 * after it. Returns `false`, without registering anything further, if
 * there are more than 8 comma-separated names — never happens in
 * well-formed keel source seen so far, but must not write out of
 * bounds. */
typedef struct { keel_slice_char name; } KKeelDeclName;

typedef struct {
    KSpecifier spec;
    KKeelDeclName names[8];
    size_t name_count;
} KKeelDecl;

bool k_scan_decl_keel(KLexer *lexer, KToken first, KSymbolTable *symtab,
                       KKeelDecl *out, KToken *next_out, TKPpKind *next_pp_kind_out);

#endif /* CGEN_ENGINE_PARSER_H */
