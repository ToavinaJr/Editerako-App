(comment) @comment
(string) @string
(template_string) @string
(number) @number
(true) @constant
(false) @constant
(null) @constant
(undefined) @constant

(function_declaration
  name: (identifier) @function)
(generator_function_declaration
  name: (identifier) @function)
(class_declaration
  name: (identifier) @type)
(property_identifier) @property
(identifier) @variable

(jsx_opening_element
  name: (identifier) @tag)
(jsx_closing_element
  name: (identifier) @tag)
(jsx_self_closing_element
  name: (identifier) @tag)

[
  "const"
  "let"
  "var"
  "function"
  "return"
  "if"
  "else"
  "for"
  "while"
  "do"
  "switch"
  "case"
  "default"
  "break"
  "continue"
  "class"
  "extends"
  "new"
  "import"
  "export"
  "from"
  "as"
  "async"
  "await"
  "typeof"
  "instanceof"
  "in"
  "of"
  "try"
  "catch"
  "finally"
  "throw"
  "void"
  "delete"
  "yield"
] @keyword
