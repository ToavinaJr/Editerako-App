(comment) @comment
(string) @string
(integer) @number
(float) @number
(true) @constant
(false) @constant
(none) @constant

(function_definition
  name: (identifier) @function)
(class_definition
  name: (identifier) @type)
(decorator) @keyword.directive
(identifier) @variable

[
  "def"
  "class"
  "return"
  "if"
  "else"
  "elif"
  "for"
  "while"
  "import"
  "from"
  "as"
  "try"
  "except"
  "finally"
  "with"
  "yield"
  "lambda"
  "pass"
  "break"
  "continue"
  "and"
  "or"
  "not"
  "in"
  "is"
  "async"
  "await"
  "global"
  "nonlocal"
  "assert"
  "del"
] @keyword
