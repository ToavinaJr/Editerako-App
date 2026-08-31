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
(raw_string_literal) @string
(char_literal) @string
(escape_sequence) @string.escape

; Numbers and constants
(number_literal) @number
(true) @constant.builtin
(false) @constant.builtin
(null) @constant.builtin
(this) @variable.builtin

; Types
(primitive_type) @type
(type_identifier) @type
(auto) @type
((namespace_identifier) @type
 (#match? @type "^[A-Z]"))

; Namespaces / modules
(namespace_identifier) @module
(module_name
  (identifier) @module)

; Functions
(call_expression
  function: (identifier) @function)
(call_expression
  function: (qualified_identifier
    name: (identifier) @function))

(template_function
  name: (identifier) @function)

(template_method
  name: (field_identifier) @function)

(function_declarator
  declarator: (identifier) @function)
(function_declarator
  declarator: (field_identifier) @function)
(function_declarator
  declarator: (qualified_identifier
    name: (identifier) @function))

; Parameters
(parameter_declaration
  declarator: (identifier) @variable.parameter)

; Fields
(field_identifier) @variable.member

; Keywords (anonymous tokens present in tree-sitter-cpp)
[
  "catch"
  "class"
  "co_await"
  "co_return"
  "co_yield"
  "constexpr"
  "constinit"
  "consteval"
  "delete"
  "explicit"
  "final"
  "friend"
  "mutable"
  "namespace"
  "noexcept"
  "new"
  "override"
  "private"
  "protected"
  "public"
  "template"
  "throw"
  "try"
  "typename"
  "using"
  "concept"
  "requires"
  "virtual"
  "import"
  "export"
  "module"
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
] @keyword

; Operators
(operator_name) @operator

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

; Variables last so more specific captures can overlap later
(identifier) @variable
