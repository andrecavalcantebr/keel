; keel — realce sobre a gramática do C (tree-sitter-c).
; GERADO por editors/gerar.py: editors/c-highlights.scm + keel-spec.md §2.2.
; Não editar à mão.
;
; O parser é o do C, então as construções de keel caem em nós de erro ou viram
; identificadores. As regras abaixo casam por GRAFIA, que é o que funciona nos
; dois casos — e é a mesma disciplina do §2.2: palavra vale pela posição, e aqui
; se realça pela escrita.

; Regras de C para tree-sitter-c v0.24.2, com capturas reconhecidas pelo Zed.
; O gerador insere o vocabulário da spec no filtro de tipos para evitar
; que uma regra genérica de C dispute a mesma palavra com o realce de keel.

(comment) @comment
(string_literal) @string
(system_lib_string) @string
(char_literal) @string
(escape_sequence) @string.escape
(number_literal) @number
[(true) (false)] @boolean
(null) @constant.builtin

(primitive_type) @type.builtin
(sized_type_specifier) @type.builtin
((type_identifier) @type
 (#not-match? @type "^(constexpr|ptrdiff_t|uintptr_t|extern_c|import_c|instance|modifier|parallel|foreach|outcome|routine|buffer|import|module|size_t|tagged|apply|arena|array|byref|corot|defer|later|match|range|slice|types|bf16|fail|priv|tags|type|walk|ALL|ANY|dim|f16|f32|f64|i16|i32|i64|now|pub|ref|u16|u32|u64|win|as|i8|u8)$"))
(field_identifier) @property
(statement_identifier) @label

(function_declarator
 declarator: (identifier) @function)
(call_expression
 function: (identifier) @function)
(call_expression
 function: (field_expression field: (field_identifier) @function))

[
 "if" "else" "switch" "case" "default"
 "for" "while" "do" "break" "continue" "goto" "return"
 "struct" "union" "enum" "typedef"
 "const" "volatile" "restrict" "static" "extern" "inline"
 "auto" "register" "sizeof" "_Alignof" "_Alignas" "_Atomic"
 "alignof" "alignas" "_Generic" "_Noreturn" "thread_local" "constexpr"
] @keyword

[
 "#include" "#define" "#if" "#ifdef" "#ifndef"
 "#elif" "#else" "#endif"
] @preproc
(preproc_directive) @preproc

["(" ")" "[" "]" "{" "}"] @punctuation.bracket
[";" "," "." "->" ":"] @punctuation.delimiter
[
 "=" "+" "-" "*" "/" "%" "++" "--"
 "==" "!=" "<" ">" "<=" ">="
 "!" "&&" "||" "&" "|" "^" "~" "<<" ">>"
 "+=" "-=" "*=" "/=" "%=" "&=" "|=" "^=" "<<=" ">>=" "?"
] @operator


; Vocabulário de keel, acrescentado às regras de C.

((identifier) @keyword
 (#match? @keyword "^(constexpr|extern_c|import_c|instance|modifier|parallel|foreach|import|module|apply|array|byref|defer|later|match|types|fail|priv|tags|type|walk|ALL|ANY|dim|now|pub|ref|win|as)$"))

((type_identifier) @keyword
 (#match? @keyword "^(constexpr|extern_c|import_c|instance|modifier|parallel|foreach|import|module|apply|array|byref|defer|later|match|types|fail|priv|tags|type|walk|ALL|ANY|dim|now|pub|ref|win|as)$"))

((identifier) @type.builtin
 (#match? @type.builtin "^(ptrdiff_t|uintptr_t|size_t|bf16|f16|f32|f64|i16|i32|i64|u16|u32|u64|i8|u8)$"))

((type_identifier) @type.builtin
 (#match? @type.builtin "^(ptrdiff_t|uintptr_t|size_t|bf16|f16|f32|f64|i16|i32|i64|u16|u32|u64|i8|u8)$"))

((identifier) @type
 (#match? @type "^(outcome|routine|buffer|tagged|arena|corot|range|slice)$"))

((type_identifier) @type
 (#match? @type "^(outcome|routine|buffer|tagged|arena|corot|range|slice)$"))
