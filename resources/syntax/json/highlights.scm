(comment) @comment
(string) @string
(string_content) @string
(number) @number
(true) @constant
(false) @constant
(null) @constant
(escape_sequence) @string.escape

(pair
  key: (string) @property)

[
  "{"
  "}"
  "["
  "]"
] @punctuation.bracket
[
  ","
  ":"
] @punctuation.delimiter
