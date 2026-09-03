# Vendored Tree-sitter runtime + grammars.
# Each grammar is an OBJECT library so scanner includes stay isolated.
# PUBLIC includes are limited to the C API consumed by the app.

set(_editerako_ts_root "${CMAKE_SOURCE_DIR}/tree-sitter")

function(editerako_ts_grammar name)
    cmake_parse_arguments(ARG "" "" "SOURCES;INCLUDES" ${ARGN})
    add_library(ts_grammar_${name} OBJECT ${ARG_SOURCES})
    target_include_directories(ts_grammar_${name} PRIVATE ${ARG_INCLUDES})
    target_compile_definitions(ts_grammar_${name} PRIVATE TREE_SITTER_NO_ICU=1)
    if(MSVC)
        target_compile_options(ts_grammar_${name} PRIVATE /W0)
    else()
        target_compile_options(ts_grammar_${name} PRIVATE -w)
    endif()
    set_target_properties(ts_grammar_${name} PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON
        C_VISIBILITY_PRESET hidden
        FOLDER "TreeSitter"
    )
endfunction()

editerako_ts_grammar(cpp
    SOURCES
        "${_editerako_ts_root}/tree-sitter-cpp/src/parser.c"
        "${_editerako_ts_root}/tree-sitter-cpp/src/scanner.c"
    INCLUDES
        "${_editerako_ts_root}/tree-sitter-cpp/src"
)

editerako_ts_grammar(html
    SOURCES
        "${_editerako_ts_root}/tree-sitter-html/src/parser.c"
        "${_editerako_ts_root}/tree-sitter-html/src/scanner.c"
    INCLUDES
        "${_editerako_ts_root}/tree-sitter-html/src"
)

editerako_ts_grammar(c
    SOURCES
        "${_editerako_ts_root}/tree-sitter-c/src/parser.c"
    INCLUDES
        "${_editerako_ts_root}/tree-sitter-c/src"
)

editerako_ts_grammar(python
    SOURCES
        "${_editerako_ts_root}/tree-sitter-python/src/parser.c"
        "${_editerako_ts_root}/tree-sitter-python/src/scanner.c"
    INCLUDES
        "${_editerako_ts_root}/tree-sitter-python/src"
)

editerako_ts_grammar(javascript
    SOURCES
        "${_editerako_ts_root}/tree-sitter-javascript/src/parser.c"
        "${_editerako_ts_root}/tree-sitter-javascript/src/scanner.c"
    INCLUDES
        "${_editerako_ts_root}/tree-sitter-javascript/src"
)

editerako_ts_grammar(typescript
    SOURCES
        "${_editerako_ts_root}/tree-sitter-typescript/typescript/src/parser.c"
        "${_editerako_ts_root}/tree-sitter-typescript/typescript/src/scanner.c"
    INCLUDES
        "${_editerako_ts_root}/tree-sitter-typescript/typescript/src"
)

editerako_ts_grammar(tsx
    SOURCES
        "${_editerako_ts_root}/tree-sitter-typescript/tsx/src/parser.c"
        "${_editerako_ts_root}/tree-sitter-typescript/tsx/src/scanner.c"
    INCLUDES
        "${_editerako_ts_root}/tree-sitter-typescript/tsx/src"
)

editerako_ts_grammar(json
    SOURCES
        "${_editerako_ts_root}/tree-sitter-json/src/parser.c"
    INCLUDES
        "${_editerako_ts_root}/tree-sitter-json/src"
)

editerako_ts_grammar(css
    SOURCES
        "${_editerako_ts_root}/tree-sitter-css/src/parser.c"
        "${_editerako_ts_root}/tree-sitter-css/src/scanner.c"
    INCLUDES
        "${_editerako_ts_root}/tree-sitter-css/src"
)

editerako_ts_grammar(bash
    SOURCES
        "${_editerako_ts_root}/tree-sitter-bash/src/parser.c"
        "${_editerako_ts_root}/tree-sitter-bash/src/scanner.c"
    INCLUDES
        "${_editerako_ts_root}/tree-sitter-bash/src"
)

editerako_ts_grammar(cmake
    SOURCES
        "${_editerako_ts_root}/tree-sitter-cmake/src/parser.c"
        "${_editerako_ts_root}/tree-sitter-cmake/src/scanner.c"
    INCLUDES
        "${_editerako_ts_root}/tree-sitter-cmake/src"
)

editerako_ts_grammar(yaml
    SOURCES
        "${_editerako_ts_root}/tree-sitter-yaml/src/parser.c"
        "${_editerako_ts_root}/tree-sitter-yaml/src/scanner.c"
    INCLUDES
        "${_editerako_ts_root}/tree-sitter-yaml/src"
)

editerako_ts_grammar(markdown
    SOURCES
        "${_editerako_ts_root}/tree-sitter-markdown/src/parser.c"
        "${_editerako_ts_root}/tree-sitter-markdown/src/scanner.c"
    INCLUDES
        "${_editerako_ts_root}/tree-sitter-markdown/src"
)

editerako_ts_grammar(sql
    SOURCES
        "${_editerako_ts_root}/tree-sitter-sql/src/parser.c"
        "${_editerako_ts_root}/tree-sitter-sql/src/scanner.c"
    INCLUDES
        "${_editerako_ts_root}/tree-sitter-sql/src"
)

add_library(tree_sitter STATIC
    "${_editerako_ts_root}/tree-sitter/lib/src/lib.c"
    $<TARGET_OBJECTS:ts_grammar_cpp>
    $<TARGET_OBJECTS:ts_grammar_html>
    $<TARGET_OBJECTS:ts_grammar_c>
    $<TARGET_OBJECTS:ts_grammar_python>
    $<TARGET_OBJECTS:ts_grammar_javascript>
    $<TARGET_OBJECTS:ts_grammar_typescript>
    $<TARGET_OBJECTS:ts_grammar_tsx>
    $<TARGET_OBJECTS:ts_grammar_json>
    $<TARGET_OBJECTS:ts_grammar_css>
    $<TARGET_OBJECTS:ts_grammar_bash>
    $<TARGET_OBJECTS:ts_grammar_cmake>
    $<TARGET_OBJECTS:ts_grammar_yaml>
    $<TARGET_OBJECTS:ts_grammar_markdown>
    $<TARGET_OBJECTS:ts_grammar_sql>
)

target_include_directories(tree_sitter
    PUBLIC
        "${_editerako_ts_root}/tree-sitter/lib/include"
    PRIVATE
        "${_editerako_ts_root}/tree-sitter/lib/src"
)

target_compile_definitions(tree_sitter PRIVATE TREE_SITTER_NO_ICU=1)

if(MSVC)
    target_compile_options(tree_sitter PRIVATE /W0)
else()
    target_compile_options(tree_sitter PRIVATE -w)
endif()

set_target_properties(tree_sitter PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    FOLDER "TreeSitter"
)

unset(_editerako_ts_root)
