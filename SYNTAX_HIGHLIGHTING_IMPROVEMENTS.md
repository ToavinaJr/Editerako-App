# C++ Syntax Highlighting Enhancement

## Overview

The syntax highlighting for C++ in Editerako-App has been significantly enhanced to provide comprehensive coloration similar to professional IDEs like Qt Creator or Visual Studio Code.

## Improvements Made

### 1. Enhanced Text Format Definitions
Added multiple new `QTextCharFormat` definitions in `syntaxhighlighter.h`:
- `functionFormat` - For function names and calls (yellow)
- `variableFormat` - For variable identifiers (light blue)  
- `preprocessorFormat` - For preprocessor directives (purple)
- `operatorFormat` - For operators (light gray)
- `punctuatorFormat` - For braces, parentheses, brackets (gold)
- `numberFormat` - For numeric literals (light green)
- `classFormat` - For class names (teal)
- `namespaceFormat` - For namespace identifiers (light blue)

### 2. Professional Color Scheme
Colors chosen to match popular IDE themes:
- **Keywords** (`#569CD6`): Blue for C++ keywords like `if`, `for`, `class`, `public`, etc.
- **Types** (`#4EC9B0`): Teal for primitive and user-defined types
- **Strings** (`#D69D85`): Light brown for string literals
- **Comments** (`#6A9955`): Green for comments (italicized)
- **Functions** (`#DCDCAA`): Yellow for function names
- **Variables** (`#9CDCFE`): Light blue for identifiers
- **Preprocessor** (`#C586C0`): Purple for `#include`, `#define`, etc.
- **Numbers** (`#B5CEA8`): Light green for numeric literals
- **Punctuation** (`#FFD700`): Gold for braces, parentheses, brackets

### 3. Recursive Tree-sitter Traversal
- Replaced the simple loop-based approach with a comprehensive recursive traversal
- Handles nested syntax structures properly
- Ensures all nodes in the syntax tree are processed

### 4. Comprehensive Node Type Mapping
The new implementation handles a wide variety of C++ Tree-sitter node types:

#### Keywords:
`if`, `else`, `for`, `while`, `do`, `switch`, `case`, `default`, `break`, `continue`, `return`, `goto`, `try`, `catch`, `throw`, `class`, `struct`, `union`, `enum`, `namespace`, `using`, `typedef`, `typename`, `template`, `public`, `private`, `protected`, `virtual`, `static`, `const`, `volatile`, `mutable`, `inline`, `extern`, `constexpr`, `decltype`, `auto`, `register`, `thread_local`, `new`, `delete`, `sizeof`, `alignof`, `noexcept`, `nullptr`, `operator`, `this`, `friend`, `explicit`, `override`, `final`

#### Operators:
Assignment (`=`, `+=`, `-=`, etc.), arithmetic (`+`, `-`, `*`, `/`, etc.), comparison (`==`, `!=`, `<`, `>`, etc.), logical (`&&`, `||`, `!`), bitwise (`&`, `|`, `^`, `~`, `<<`, `>>`, etc.), member access (`->`, `.`, `::`), ternary (`?`, `:`), and expression types

#### Preprocessor Directives:
`#include`, `#define`, `#ifdef`, `#ifndef`, `#if`, `#else`, `#elif`, `#endif`, `#line`, `#error`, `#warning`, `#pragma`

### 5. UTF-8/UTF-16 Position Handling
- Proper conversion between Tree-sitter byte positions and Qt character positions
- Handles multi-byte UTF-8 characters correctly
- Ensures accurate text highlighting in the Qt text editor

### 6. Tree-sitter Parser Integration
- Updated CMakeLists.txt to build tree-sitter-cpp and tree-sitter-html from source
- Fallback system if system libraries aren't available
- Proper error handling and warnings for missing language parsers

## Before vs After

### Before (Limited Highlighting):
- Only highlighted: `string_literal`, `primitive_type`, `function_definition`, `comment`
- Simple loop through direct children only
- Many C++ elements remained uncolored

### After (Comprehensive Highlighting):
- Keywords, types, functions, variables, operators, preprocessor directives
- Numbers, strings, comments, punctuation
- Class names, namespaces, identifiers
- Recursive traversal covers all syntax tree nodes
- Professional IDE-like color scheme

## Technical Details

### Key Functions:
- `setupFormats()`: Defines all color formats with professional color scheme
- `highlightNodeRecursive()`: Recursively traverses Tree-sitter syntax tree
- `highlightCpp()`: Main entry point, parses text and starts recursive highlighting

### Position Conversion:
```cpp
// Convert byte positions to character positions for Qt
QByteArray textBytes = text.toUtf8();
int startChar = QString::fromUtf8(textBytes.left(static_cast<int>(startByte))).length();
int endChar = QString::fromUtf8(textBytes.left(static_cast<int>(endByte))).length();
```

### Node Type Matching:
Uses comprehensive string matching to identify Tree-sitter node types and apply appropriate formatting.

## Building

The enhanced syntax highlighter requires:
- Qt5/Qt6 development libraries
- tree-sitter library
- tree-sitter-cpp parser (built from source if not available)

Build with:
```bash
mkdir build && cd build
cmake ..
make
```

The system will automatically build the required Tree-sitter parsers from the included submodule sources.