; Comments
(comment) @comment

; Preprocessor
(preproc_directive) @keyword.directive
(preproc_include) @keyword.directive
(preproc_def) @keyword.directive
(preproc_function_def) @keyword.directive
(preproc_call) @keyword.directive
(preproc_if) @keyword.directive
(preproc_ifdef) @keyword.directive
(preproc_else) @keyword.directive
(preproc_elif) @keyword.directive
(preproc_defined) @keyword.directive

; Strings
(string_literal) @string
(char_literal) @string
(escape_sequence) @string.escape

; Numbers and constants
(number_literal) @number
(true) @constant.builtin
(false) @constant.builtin
(null) @constant.builtin

; Types
(primitive_type) @type
(type_identifier) @type

; Functions
(call_expression
  function: (identifier) @function)
(function_declarator
  declarator: (identifier) @function)
(function_declarator
  declarator: (field_identifier) @function)

; Parameters
(parameter_declaration
  declarator: (identifier) @variable.parameter)

; Fields
(field_identifier) @variable.member

; Keywords
[
  "if"
  "else"
  "for"
  "while"
  "do"
  "switch"
  "case"
  "default"
  "return"
  "break"
  "continue"
  "goto"
  "struct"
  "enum"
  "union"
  "const"
  "static"
  "extern"
  "inline"
  "sizeof"
  "typedef"
  "volatile"
] @keyword

; Punctuation
[
  "{"
  "}"
  "("
  ")"
  "["
  "]"
] @punctuation.bracket
[
  ";"
  ","
] @punctuation.delimiter

(identifier) @variable
