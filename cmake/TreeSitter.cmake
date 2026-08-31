# Vendored Tree-sitter runtime + C++ / HTML grammars.
# PUBLIC includes are limited to the C API consumed by the app.

set(_editerako_ts_root "${CMAKE_SOURCE_DIR}/tree-sitter")

add_library(tree_sitter STATIC
    "${_editerako_ts_root}/tree-sitter/lib/src/lib.c"
    "${_editerako_ts_root}/tree-sitter-cpp/src/parser.c"
    "${_editerako_ts_root}/tree-sitter-cpp/src/scanner.c"
    "${_editerako_ts_root}/tree-sitter-html/src/parser.c"
    "${_editerako_ts_root}/tree-sitter-html/src/scanner.c"
)

target_include_directories(tree_sitter
    PUBLIC
        "${_editerako_ts_root}/tree-sitter/lib/include"
    PRIVATE
        "${_editerako_ts_root}/tree-sitter/lib/src"
        "${_editerako_ts_root}/tree-sitter-cpp/src"
        "${_editerako_ts_root}/tree-sitter-html/src"
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
)

unset(_editerako_ts_root)
