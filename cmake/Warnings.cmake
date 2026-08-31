# Compiler warnings and MSVC flags required by Qt headers.
# Not applied to vendored Tree-sitter.

function(editerako_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /utf-8
            /Zc:__cplusplus
            /permissive-
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
    endif()
endfunction()
