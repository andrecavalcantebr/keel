# Mensagens dos diagnósticos do cgen

Este arquivo é a fonte das mensagens que o cgen imprime, e de uma descrição em
inglês de cada condição. O `gen-diags.py` o lê junto com as tabelas dos
normativos e gera `src/engine/diag_messages.def`; nada aqui é escrito no
código à mão.

A divisão de propriedade:

- **Os normativos são a autoridade** sobre o identificador, a severidade e a
  condição: a [tabela da spec](../../keel-spec.md#62-catálogo) (§6.2), para os
  da linguagem, e a [da ferramenta](../../cgen-tool-spec.md#71-diagnósticos-da-ferramenta)
  (§7.1), para os da invocação. A seção `cgen` traz os identificadores que são
  da implementação, e que nenhum normativo lista.
- **Este arquivo é a autoridade** sobre o texto: a mensagem (`Message`) e a
  condição traduzida (`Condition`). Ele não é normativo, e ajustar uma
  mensagem não pede mudança de spec.

As regras, que o `gen-diags.py --check` verifica:

- cada seção tem os identificadores da sua tabela, na mesma ordem, nem mais nem
  menos;
- `Message` é o texto que sai no diagnóstico: cada `%s` recebe o próximo
  argumento, e `\|` é o `|`. Vazia, o diagnóstico ainda não é emitido, e o cgen
  diz isso se chegar a emiti-lo. As linhas `debug` não têm mensagem, porque são
  verificações do programa gerado, não da tradução. Os da §7.1 que o `tool/`
  imprime direto (com mensagem por chamada) também ficam sem;
- `Condition` é a tradução da condição do normativo, e é o que o
  `--cgen-help=<classe>` mostra. Quando a condição muda no normativo, muda aqui.

A manutenção: quando um normativo ganha, perde ou muda uma linha,
`tools/cgen/diag-sync.py` pede ao Claude (Haiku 5.5) a condição e a mensagem das
linhas novas ou alteradas, reescreve este arquivo e roda o `gen-diags.py`. O
`diag-messages.lock.json` guarda o hash da condição em pt-BR com que cada linha
foi sincronizada, e é por ele que o script sabe o que mudou. Ajustar uma
mensagem à mão continua valendo: o script só reescreve as linhas que pergunta.
O resultado se revisa com `git diff`. Os detalhes estão em `diag-sync.py --help`.

O conteúdo das tabelas é em inglês, como o código.

## keel-spec §6.2

| Identifier | Message | Condition |
| --- | --- | --- |
| `missing-module` | a keel file starts with `module NAME;`, before any directive or declaration | A file whose first significant token is not `module` |
| `module-path-mismatch` | expected module `%s` for this path, found `%s` | The name declared in `module` differs from the path relative to the root |
| `invalid-stem` | the file name `%s` is not a C identifier, so it cannot name a module: rename the file | A file stem that is not a valid C identifier |
| `case-ambiguous-stem` | `%s` and `%s` differ only in case: on a case-insensitive file system they are one file; rename one | File names that differ only in case |
| `symbol-collision` | `%s`, the C name of `%s`, is also a name exported by `%s`: rename one of them | Exported symbols collide in the module and the transitive closure of its imports |
| `circular-import` | module `%s` is already being loaded: importing it here closes a cycle | A circular import, with the whole chain in the message |
| `nested-extern-c` | `extern_c` stands inside a declaration: it is a file-scope construction | `extern_c` in a nested position, that is, outside file level |
| `type-layer-on-priv-extern-c` | `priv extern_c [type_h]` is contradictory: `type_h` puts the text in the interface, `priv` in the implementation | `priv extern_c [type_h]`: contradictory qualifiers |
| `main-in-extern-c` | `main` is defined inside `extern_c`: `main` is a function of the module, which takes its prefix and gets the C wrapper | `main` defined inside `extern_c` |
| `private-main` | `main` is `priv`: `main` is a public function of the module | `priv` applied to `main` |
| `invalid-main-signature` | `main` has neither form of C: write `int main(void)` or `int main(int argc, char *argv[])` | `main` declared outside the two forms of C |
| `import-clause-order` | `as` comes before `types`: write `import %s as %s types;` | `import M types as m;`: the message gives the correct form |
| `duplicate-injected-name` | `%s` comes bare from `%s` and from `%s`: import one of them without `types`, and write the name qualified | Two imports with `types` injecting the same bare name |
| `duplicate-alias` | `%s`, the alias of `import %s`, is already the qualifier of `%s`: each import needs its own | Two imports with the same alias, or an alias equal to the qualifier of another import (the implicit `keel` included); the message cites both imports |
| `shadowed-injected-name` | `%s` hides the type that `types` brought from `%s`: the bare name now means this declaration | A name injected by `types` is shadowed by a local declaration |
| `injected-names` | `types` brings from `%s`, unqualified: %s | The list of names that `types` injected, at the import |
| `pub-static` | `pub static` without `inline`: a public symbol cannot have internal linkage; drop `static` or make it `priv` | `pub static` without `inline` at file scope |
| `inline-without-visibility` | `static inline` at module level needs `pub` or `priv`: say whether it belongs to the interface | `static inline` without `pub`/`priv` at module level |
| `static-on-type` | `static` does not apply to a type | `static` applied to a type |
| `name-too-long` | generated name exceeds the backend limit of 255 characters | A generated name above the length ceiling of the target |
| `not-a-container-expression` | `%s` takes a container as its object, and `%s` is not one: name a symbol declared with a keel type (or an index, field or verb over one) | An expression outside the container grammar in container position |
| `c-type-as-argument` | `%s` takes keel types as arguments, and `%s` is a C keyword: write the keel spelling (`i32`, `u8`, `f64`, ...) or a named type | A C arithmetic type keyword as an argument of a modifier, except `char` and `bool`, instead of the keel spelling or a named type |
| `reserved-name` | `%s` is in the space of the generated C: names starting with `keel_` or `KEEL_` are reserved | An identifier declared by the program in the reserved spaces `keel_` and `KEEL_` |
| `symbol-redeclaration` | `%s` is already a keel symbol, and here it may be declared again (`%s %s`): keel does not decide which, and refuses; use another name | A local pattern of possible redeclaration of a known symbol (spec §2.5) |
| `buffer-over-const` | `%s` cannot make a `buffer` over the const elements `%s`: use `slice.of`, which gives a `slice const T` | `buffer` over a `const` C vector: the message points to `slice const T` |
| `element-copy-in-get-set` | `%s` would copy an element that is itself `%s`; take its address with `ptr` instead | `get` or `set` over an element that is an instance of a modifier |
| `index-out-of-length` |  | `get` or `ptr(x, i)` (and through them the sugar `x[i]`) with an index outside `length`, over `buffer` or `slice` |
| `set-out-of-length` |  | `set` with an index outside `length` |
| `buffer-of-unknown-size` | `%s` needs a one-dimensional `array` to read the size from, and `%s` is not one: use `buffer.of(p, n)` | One-argument `buffer.of` over a symbol that is not an `array` |
| `array-1d-as-parameter` | the one-dimensional `array` parameter `%s` has no binder: write `array T name[size_t n]` so the function receives its length | A one-dimensional `array` function parameter without a binder |
| `array-parameter-without-dimension` | the parameter `%s` is an `array` without a dimension: write `array T name[size_t n]` to bind its length | `array T v[]` without a dimension in a parameter |
| `array-argument-wrong-dimension` | `%s` has dimensions %s, and the parameter declares %s: the rank and the dimensions from index 1 on coincide, and dimension 0 is not smaller | An `array` argument with a different rank, a different dimension from index 1 on, or dimension 0 smaller than the parameter's |
| `partial-array-index` | the array has dimensions %s: index every one of them (`v[i,j]` or `v[i][j]`); a partial index has no meaning | Partial indexing of a multidimensional `array` |
| `flat-view-of-n-dim-array` | `%s` needs a one-dimensional `array`, and `%s` has dimensions `%s`: index it down to one dimension first | `buffer.of`, `slice.of` or `x[a..b]` over a multidimensional `array` |
| `binder-argument-not-array` | `%s` carries no dimension: the parameter binds its dimension 0, so the argument is an `array` symbol or field | The argument of a parameter with a dimension binder is not an `array` symbol or field, an `extent` column included |
| `binder-as-dimension` | `%s` is the binder of an `array` parameter, not a constant: it cannot size a vector | A dimension binder used as the dimension of a vector |
| `nonconstant-dim-index` | `keel.dim` needs a dimension index known at translation time, and `%s` is not one: write a decimal literal or a `constexpr` | The index of `keel.dim(v,k)` has no known decimal value |
| `ref-without-initializer` | `ref` symbol `%s` has no initializer: a `ref` is initialized where it is declared | `ref` without an initializer |
| `ref-arithmetic` | `%s` is a `ref`: it names one element and takes no arithmetic or index; use a plain pointer to walk | A binary additive operator, `+=`, `-=`, increment, decrement or index applied to a `ref` symbol; includes `q - p` with `p` a `ref`, whatever the type of `q` |
| `slice-from-ref` | `slice.from` over `%s`, a `ref`: keel does not vouch for the extent behind a `ref`; pass a pointer | `slice.from` over a `ref` |
| `region-escape` | `%s` is returned, and its storage comes from an `array` local to the function: it ends with the function | Return of a symbol whose provenance includes automatic storage local to the function |
| `defer-without-braces` | `defer` is the whole body of `%s`: put it in braces, so the scope it cleans up is the block's | `defer` as the body of a control statement without braces |
| `defer-at-file-scope` | `defer` at file scope: a `defer` belongs to a block of a function | `defer` at file scope |
| `jump-over-defer` | `%s` enters a block past a `defer` registered in it: the cleanup would run for a registration that never happened; give the case its own block, or move the label | An outside entry into a scope past a `defer` registration, including a later `case`/`default` in the same `switch` body |
| `later-with-capture` | `later` reads the values at the exit and takes no list (`%s`): write `[now ...]` to copy them | `later` followed by a capture list, inside the brackets of `defer` |
| `unmatched-delimiter` | `%s` has no pair: it meets %s | A brace, parenthesis or bracket without a pair, inside `extern_c` too |
| `unexpected-eof` | the file ends inside a declaration: `%s` is the last token, and the declaration has no `;` or closing brace | End of file while recognizing a candidate form, with no terminating token and no open delimiter |
| `delimiter-mismatch-across-branches` | the alternatives of this conditional group leave different delimiters open: every branch, the missing `#else` included, has to open and close the same | The alternatives of a conditional group disagree on the count of delimiters |
| `define-over-keel-name` | cannot `#%s` `%s`: %s | `#define` or `#undef` of a `keel_` or `KEEL_` name |
| `define-over-keel-word` | `#%s` of `%s`, a keel word: where it stands as a construction, keel reads the construction, not the macro | `#define` or `#undef` of a contextual keel word, except the role words |
| `literal-with-newline` | %s literal has no closing `%s` before the end of the line; write `\n` for a newline inside it | A string or char literal with an unspliced newline |
| `keel-name-shadowed` | `%s` is a keel word or the qualifier of an import: where it stands as a construction, keel still reads the construction | A declaration named as a contextual keel word, except the role words, or as an import qualifier of the file |
| `byref-assignment` | `%s = %s` copies a `byref` instance: both now share the storage, and a change through one is seen through the other | Assignment between instances of a `byref` modifier, naming the aliasing |
| `defer-in-control-block` | `defer` in the body of `%s` runs when that block ends, not when the function does | `defer` registered in the body of `if`, `else` or `switch` |
| `instance-field-access` | `%s` is a field of `%s`, an instance of `%s`: the fields belong to its module; use its verbs | Direct access to a field of a modifier instance |
| `indirect-import` |  | Use of a symbol of a module that is not imported directly |
| `modifier-outside-generic` | modifier `%s` in a module with no parameters: a modifier belongs to a generic module (`module M type T;`) | `modifier` outside a generic module |
| `parameter-name-reuse` | `%s` is a parameter of the module: a declaration cannot reuse its name | A declaration of a symbol, or a dimension binder, with the name of a parameter of the module |
| `circular-generic` | the module instantiates itself without end: %s | A generic module that instantiates itself, with the chain in the message |
| `layout-cycle` | the type holds itself by value, through an instance: %s; hold it by pointer somewhere in the chain | A chain of types that contain each other by value through a modifier instance, with the chain in the message |
| `protocol-on-parameter` | `%s` over `%s`, whose type is a parameter of the module: inside the generic that type is opaque, and takes no keel construction | A keel construction applied to a value whose type is a parameter of the generic module, with the construction in the message |
| `instance-outside-file-scope` | `instance %s` stands in a function: `instance` is a file-scope declaration | `instance` outside file scope |
| `instance-not-modifier` | `instance %s`: `instance` takes a modifier of a generic module with all its arguments, as a declaration writes it | An argument of `instance` that is not a modifier of a generic module |
| `redundant-instance` | `instance %s` places no body: every function of the generic is `inline`, and comes out where it is used | `instance` over a generic that is entirely `pub inline` |
| `nonparametric-out-of-line` | `%s` mentions no parameter and no modifier of the generic, so it belongs to the module, which is emitted once: make it `inline`, a type or a `constexpr` | An out-of-line function or a variable in a generic declaration that mentions neither a parameter nor a modifier |
| `byref-param` | the parameter `%s` takes `%s` by value, and that type is `byref`: take it by pointer | A `byref` instance by value in a parameter: `arena`, `buffer` and every marked modifier |
| `child-region-after-invalidation` | `%s` is used after `%s` invalidated the storage it comes from | A recognized use of a symbol invalidated by `invalidates` or `consumes`, descendants included |
| `unnamed-tags` | a set of tags needs a name: `tags NAME [ ... ];` | `tags` without a name |
| `duplicate-tags-name` | a second set of tags named `%s` in the module | Two sets of tags with the same name in the module |
| `empty-tags` | the set `%s` has no tags | A set of tags with no tag |
| `partial-tag-values` | the set `%s` writes values for some tags and not for others: write all of them, or none | A set that mixes tags with and without a written value |
| `nonconstant-tag-value` | the value `%s` of `%s` is neither a decimal literal nor a known constant | A tag value that is not a decimal literal or a known constant |
| `undeclared-tags` | `%s` in `%s` names no set of tags: the `tags` parameter takes the name of a `tags` declaration | An argument of a `tags` parameter that names no declared set |
| `tag-from-other-set` | `%s` is a tag of another set: here the set is `%s` | A written tag that does not belong to the required set: a `match` label, or a constant in a verb with a `tags` parameter |
| `duplicate-tag` | `%s` labels a second arm of the same `match` over `%s` | A tag repeated in the same set, or a label repeated in the same `match` |
| `match-without-tags` | the `tag` of `%s` returns no set of tags, so `match` has no labels to check | An operand of `match` whose `tag` returns no `tags` set |
| `invalid-fault-code` | the fault code `%s` is not positive: a fault is a code above zero (zero is ongoing, negative is success) | `corot.fault(r,c)` with a known code that is zero or negative |
| `tag-out-of-range` |  | A tag outside the declared list |
| `no-ptr-for-arity` | `%s` has no `ptr` for the indices `%s`: a container is indexed with the arities its module declares `ptr` for | An index of two or more values over a modifier without a vector `ptr`, or whose `dim` differs from the number of indices: the message says what the module declares |
| `walk-without-cursor` | `walk` needs two binders, the element and the cursor, and `%s` is one: write `walk (T *e, M.cursor c : x)` | `walk` without the cursor binder |
| `cursor-type-mismatch` | the cursor binder is `%s`, and `begin` produces `%s` | The type of the cursor binder differs from the declared product of `begin` |
| `partition-type-mismatch` | the partition binder is `%s`, and the `partition` of `%s` produces `%s` | The type of the partition binder differs from the declared product of `partition` |
| `binder-copies-container` | the binder `%s` copies an instance of `%s` for each element: write it by pointer (`T *`) | A by-value binder of `foreach` or `walk` over an element that is a modifier instance: the message points to `T *`. Does not apply to the partition binder of `parallel` |
| `mutation-during-traversal` | `%s` changes `%s` while the `%s` goes over it: change the container before or after | `push`, `pop` or `clear` over the container being traversed or partitioned, in the body of `foreach`, `walk` or `parallel` |
| `index-not-size-t` | the index binder `%s` must be `size_t` | An index binder whose type is not `size_t` |
| `inverted-range-index` | the range `[%s]` starts above where it ends; the empty range is `[a..a]` | A range index with known decimal bounds whose start is above its end |
| `range-index-out-of-bounds` |  | A range whose bounds break `a <= b <= length(x)` |
| `array-index-above-dimension` | index `%s` is at or above its dimension in %s | A known decimal `array` index above the declared dimension |
| `array-index-out-of-bounds` |  | An `array` index outside the declared dimension or, in a parameter with a binder, the received one |
| `foreach-two-binders-on-literal` | `foreach` over the literal `%s` takes one binder: name the range to have the position | Two-binder `foreach` over a range literal: the message says to name the range |
| `pointer-binder-on-range` | `%s` is a pointer, and a range binder is the counter itself: write it by value | A pointer binder in the range form |
| `open-range-outside-index` | a range with an open end stands only in an index (`x[a..]`): write both ends here | A range with an open end outside an index |
| `open-range-index-on-complex-path` | `%s` leaves the end open on a path that holds an index or a verb, and the path would be evaluated twice: name the length | `x[a..]` over a path that holds an index or a verb |
| `enum-constant-without-type` | `%s` skips the level of the set: from outside the module, write `M.Set.%s` | An enum constant, or a tag, written without the level of the type or of the set, where it is required |
| `alias-type-collision` | `%s` is the alias of a module and also a type from `%s`: choose another alias | A module alias and a type from different origins have the same spelling in the file |
| `unnamed-parallel` | `parallel` has no name: write `parallel NAME POLICY (...)`; the name is the control symbol the queries read | `parallel` without a name |
| `flow-verb-outside-parallel` | `%s;` is a worker exit, and it stands outside the body of a `parallel` | A flow `win` or `fail` outside the body of a worker |
| `nested-parallel` | `parallel %s` stands in the body of `parallel %s`: a worker does not start workers | `parallel` nested in the body of a `parallel` |
| `duplicate-parallel-name` | `%s` names a second `parallel` in the same function: each one needs its own name | Two `parallel` with the same name in the same function |
| `nonconstant-parallel` | `%s` is not known at translation time: the worker count and the policy of `parallel` are a decimal literal or a `constexpr` | The `k` or the policy of `parallel` is not a translation-time constant |
| `tag-not-in-set` | `%s` is not a tag of `%s` | A label that is not in the declared list of the set |
| `tag-without-label` | `%s` of `%s` has no arm in this `match`: every tag of the set has a label | A tag of the declared list without a matching label in the body |
| `par-target-above-total` |  | The target of `routine.par` is above the number of slots |
| `mask-above-64-slots` |  | `routine.mask` over a slice with more than 64 entries |
| `restrict-on-container` | `restrict` does not apply to the type `%s`: qualify a pointer to it (`T *restrict p`) | `restrict` written before a modifier: the `note` gives the form with a pointer |
| `specific-format-unavailable` |  | The module uses `f16` or `bf16`, and the target does not offer the format |
| `nonconstant-dim` | the dimension `%s` of `%s` is not known at translation time: write a decimal literal or a `constexpr` | An argument of `dim` that is not a decimal literal or a `constexpr` with a known decimal initializer |
| `dim-generates-declaration` |  | `dim` used to generate declarations instead of replacing a number |
| `alloc-overflow` |  | `arena.alloc` whose `n * sz` does not fit in `size_t` |
| `canonical-name-collision` | `%s` is the C name of both `%s` and `%s` in this module: rename one | Two declarations of the same module that produce the same canonical name |
| `modifier-named-instance` | a modifier cannot be named `instance`: the word opens the declaration that places an instance's bodies | A modifier declared with the name `instance` |
| `address-in-object-position` | `%s` takes its object without `&`: the object is written as itself and the address is inserted from the parameter (`%s`) | The address operator over the object in the first argument of a verb |
| `wrong-qualifier` | the object of `%s` is a type of module `%s`, not of the module the qualifier names: write `%s.<verb>` with the module of the object | A qualifier incompatible with the receiver or the product of the verb |
| `verb-not-in-instance` | `%s` is not in the instance `%s`: its argument removes it (%s) | A recognized call to a verb unavailable in the instance; points to the call and gives the argument, the forbidden operation and, when there is one, the call chain that spread the unavailability |
| `from-without-target` | `%s` takes its type from the target, and here there is none: use it to initialize, to assign a known symbol, or in a `return` | A verb that depends on the type of the target, outside an initialization, an assignment to a known symbol or a return |
| `captured-write` | `%s` is captured by `parallel %s`, and each worker writes its own copy: pass a pointer to give a result back | An assignment to a captured scalar in the body of a `parallel` |
| `else-without-initializer` | the `else` of a result tests the value just stored, and this declaration stores none: write `T x = e else ...` | An `else` clause in a declaration without an initializer |
| `else-on-complex-target` | the `else` of a result needs a name declared with a keel type as its target, and `%s` is not one: store the result in a local first | An `else` clause over a target that is not an identifier declared in keel: a field, an index, a deref, a cast or a C symbol |
| `else-multiple-declarators` | the `else` of a result takes one declarator: declare the others in their own statement | An `else` clause in a declaration with more than one declarator |
| `hidden-declarator` | the name `%s` is not the last token of its declarator, and keel has to rebuild the declaration here: name the type with a `typedef` | A declarator whose name is not the last token, where keel has to rebuild the declaration (`constexpr`, `[now]` capture, return type under `defer`); the `note` says to use a `typedef` |
| `nonscalar-constexpr` | a `constexpr` is a scalar: no array declarator and no braces around the value; use `static const` for an aggregate | `constexpr` with an array declarator or an initializer in braces |
| `return-in-parallel` | `return` in the body of `parallel %s`: the worker does not leave the function; end it with `win;` or `fail;` | `return` in the body of a `parallel`: the worker body does not leave the enclosing function |
| `defer-later-shadowed` | `%s` is declared again inside the scope of a `defer` that reads it at the exit, and `%s` leaves from here: the cleanup would read the inner one; write `defer [now ...]` or leave by `goto` | `defer` without `[now]` whose body names a symbol redeclared in an inner scope with an exit point; the `note` gives both ways out, `[now]` or `goto` |
| `constexpr-as-lvalue` | `%s` is a `constexpr`: it has no address and cannot be assigned; use `static const` for an object | `&` over a `constexpr` symbol, or a use that needs an lvalue; the `note` gives the way out, `static const T k = K;` |
| `constexpr-name-missing` | the token before the `=` of a `constexpr` is `%s`, not the name: write `constexpr T NAME = value;`, with a `typedef` for a complex type | There is a `=` at the outer level of `decl-constexpr`, and the token right before it is not an `IDENT` |
| `dim-below-one` | the dimension `%s` of `%s` is below 1 | An argument of `dim` that resolves to a value below 1: the message gives the instantiation chain |
| `extent-count-not-field` | the count `%s` of `extent struct %s` names no field of the struct | The count of an `extent` group names no field of the struct |
| `extent-unknown-capacity` | the capacity `%s` of `extent struct %s` is not a field, a known `constexpr` or a decimal literal | The capacity of an `extent` group is not a field, a known `constexpr` or a decimal literal |
| `extent-without-column` | `extent struct %s` has no column: mark the columns with `array` | `extent` without a field marked `array` |
| `extent-mixed-columns` | `extent struct %s` mixes embedded columns and columns by pointer: use one kind | Embedded columns and columns by pointer in the same `extent` |
| `extent-embedded-field-capacity` | the embedded column `%s` needs constant capacities, and `%s` is a field: use a `constexpr`, or a column by pointer | An embedded column under a capacity that is a field |
| `extent-dimension-mismatch` | the embedded column `%s` declares `%s`, and the capacities of the groups are `%s`: one dimension per group, named as its capacity | An embedded column whose dimensions are not, in number and by name, the capacities of the groups |
| `extent-index-arity` | `%s` is a column of rank %s, and the access writes %s indices | A column access with a number of indices different from the rank |
| `extent-index-above-capacity` | index `%s` is at or above the capacity `%s` of its group | A known decimal index at or above the known decimal capacity of the group |
| `extent-path-with-call` | the column access `%s` goes through a call: store the struct, or a pointer to it, in a local first | A column access through a path that holds a call |
| `extent-index-out-of-bounds` |  | A column index outside the count, or a count above the capacity |
| `partial-instance-selection` | `%s` selects the instance, and so names a `type` parameter for every `type` of the module line (and the module has no `dim` or `tags`) | A selection verb without a `type` parameter for some parameter of the `module` line |
| `type-param-outside-size` | the erased type parameter `%s` stands only in `sizeof(%s)` and `alignof(%s)` in the body, and as a pointee in the signature | An erased `type` parameter used outside `sizeof`, `alignof` and `X *` in the signature, or in a vector dimension |
| `type-param-shadows-type` | the erased type parameter `%s` has the name of a known type: choose another name | An erased `type` parameter with the name of a type known in scope |
| `role-position` | `%s` is a role: a role stands before the type of a parameter, and only `child` before the return type | A role outside an admitted position |
| `unknown-protocol` | `%s` names no protocol here: declare it, or import the module that does | An unknown protocol name |
| `associated-type-unbound` | the associated type `%s` of `%s` appears in no verb, so nothing binds it | An associated type that appears in no required verb of the protocol |
| `associated-type-conflict` | the associated type `%s` binds to `%s` through one verb and to `%s` through another: it binds once | Incompatible bindings for the same associated type |
| `circular-protocol` | the composition of `%s` comes back to itself through `%s` | A cycle in the composition of protocols |
| `protocol-verb-conflict` | the protocol requires `%s` twice with the same arity and different signatures (in `%s`) | Verbs with the same name and arity and conflicting signatures |
| `protocol-role-mismatch` | `%s` declares roles that differ from the prototype in `%s`: write the same roles in the same positions, or none, and the prototype's apply | The roles of the implementer differ from the required ones |
| `not-in-v0` | `%s` is a v1 construction: this implementation recognizes it and does not translate it | A normative v1 construction recognized by a v0 implementation |
| `mold-position` |  | (v1) A call to a mold in a position that is not admitted |
| `mold-return` |  | (v1) A mold return outside the allowed final form |
| `mold-argument` |  | (v1) A mold argument outside the allowed category |
| `circular-mold` |  | (v1) A cycle of mold expansion |
| `mold-depth` |  | (v1) Expansion depth above the limit |
| `mold-size` |  | (v1) Number of expanded tokens above the limit |
| `code-param-outside-mold` |  | (v1) A `keel_code` parameter in a function that is not a mold |
| `protocol-not-satisfied` | `%s` does not meet `%s`: its module does not declare %s (an `array` takes the verbs from `keel.array`, which must be imported) | A type that does not meet the protocol required by the construction or by the function parameter: the module does not declare the verbs, or the instance lost them. The message names the construction, the protocol and the missing verbs; over `array`, it points to the import of `keel.array` |
| `verb-not-in-protocol` | `%s` is not a verb of `%s`: a function over a protocol calls on its parameter only the verbs the protocol requires | A verb outside the contract of the protocol parameter |
| `instance-depth` |  | Specialization depth above the limit |

## cgen-tool-spec §7.1

| Identifier | Message | Condition |
| --- | --- | --- |
| `multiple-sources` |  | More than one source in the same invocation: two `.k`, or a `.k` and an `--instance` |
| `generic-source-without-instance` |  | The `.k` of the invocation is a generic module, and there is no `--instance` |
| `invalid-instance` |  | An `--instance` that does not name a modifier of a generic module in full, or whose arguments do not match its parameters |
| `base-not-found` |  | None of the base roots holds `keel.k` |
| `source-not-found` |  | The named `.k` does not exist or cannot be read |
| `source-outside-roots` |  | The `.k` is under no `-I` root |
| `source-in-multiple-roots` |  | The `.k` is under more than one root |
| `module-not-found` | module `%s` was not found in the source roots | An `import` that no root resolves |
| `missing-entry-point` |  | `--main` over a module without a public entry function |
| `lowering-unavailable` |  | `--parallel-lowering=openmp` without `-fopenmp` on the line |
| `invalid-option` |  | A cgen option with a value outside its enumeration |
| `output-with-gen` |  | `-o` with `--stop-after=gen`: the phase writes several files; the message points to `--dest-dir` |
| `write-failure` |  | I/O failed while writing under `--dest-dir` |
| `implementation-limit` | insufficient storage while collecting module `%s` | A configured capacity of the implementation was reached; the message gives the limit, its value, the demand when known and the setting to adjust |

## cgen

| Identifier | Message | Condition |
| --- | --- | --- |
| `unexpected-token` | malformed or truncated declaration | A token that no form of the parser accepts at this point: a malformed or truncated declaration |
| `module-load-failed` | cannot load module `%s` | A module of the import closure could not be loaded |
