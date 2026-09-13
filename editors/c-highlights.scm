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
 (#not-match? @type "^(__KEEL_WORDS__)$"))
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
