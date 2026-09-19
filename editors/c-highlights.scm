; C rules for tree-sitter-c v0.24.2, with captures Zed recognizes.
; The generator inserts the spec's vocabulary into the type filter so that a
; generic C rule does not compete with keel's highlighting for the same word.

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
