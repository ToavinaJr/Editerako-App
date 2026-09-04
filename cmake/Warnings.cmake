# Compiler warnings and MSVC flags required by Qt headers.
# Not applied to vendored Tree-sitter.

option(EDITERAKO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors (CI)" OFF)

function(editerako_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /utf-8
            /Zc:__cplusplus
            /permissive-
        )
        if(EDITERAKO_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
        if(EDITERAKO_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()
