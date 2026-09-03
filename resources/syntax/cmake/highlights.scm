(line_comment) @comment
(bracket_comment) @comment
(quoted_argument) @string
(bracket_argument) @string
(normal_command
  (identifier) @function)
(identifier) @variable
