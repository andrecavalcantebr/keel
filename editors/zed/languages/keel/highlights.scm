; keel — realce sobre a gramática do C (tree-sitter-c).
; GERADO por editors/gerar.py a partir de keel-spec.md §3.7. Não editar à mão.
;
; O parser é o do C, então as construções de keel caem em nós de erro ou viram
; identificadores. As regras abaixo casam por GRAFIA, que é o que funciona nos
; dois casos — e é a mesma disciplina do §3.7: palavra vale pela posição, e aqui
; se realça pela escrita.

((identifier) @keyword
 (#match? @keyword "^(constexpr|extern_c|import_c|instance|modifier|parallel|coagain|cobreak|foreach|cofail|import|module|apply|array|byref|cofsm|copar|coseq|cowin|defer|later|types|priv|type|ALL|ANY|dim|now|pub|ref|as)$"))

((type_identifier) @keyword
 (#match? @keyword "^(constexpr|extern_c|import_c|instance|modifier|parallel|coagain|cobreak|foreach|cofail|import|module|apply|array|byref|cofsm|copar|coseq|cowin|defer|later|types|priv|type|ALL|ANY|dim|now|pub|ref|as)$"))

((identifier) @type.builtin
 (#match? @type.builtin "^(ptrdiff_t|uintptr_t|size_t|bf16|f16|f32|f64|i16|i32|i64|u16|u32|u64|i8|u8)$"))

((type_identifier) @type.builtin
 (#match? @type.builtin "^(ptrdiff_t|uintptr_t|size_t|bf16|f16|f32|f64|i16|i32|i64|u16|u32|u64|i8|u8)$"))

((identifier) @type
 (#match? @type "^(outcome|buffer|strbuf|string|arena|corot|range|slice)$"))

((type_identifier) @type
 (#match? @type "^(outcome|buffer|strbuf|string|arena|corot|range|slice)$"))
