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
(class_declaration
  name: (type_identifier) @type)
(interface_declaration
  name: (type_identifier) @type)
(type_identifier) @type
(property_identifier) @property
(identifier) @variable

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
  "interface"
  "type"
  "implements"
  "public"
  "private"
  "protected"
  "readonly"
  "declare"
  "namespace"
  "abstract"
  "enum"
  "try"
  "catch"
  "finally"
  "throw"
] @keyword
