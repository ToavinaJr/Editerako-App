# AddressSanitizer / UndefinedBehaviorSanitizer / ThreadSanitizer.
# ASan and TSan cannot share an executable. Use separate presets (asan / tsan).
# Not applied to MSVC or MinGW GCC (unsupported / unreliable).

option(EDITERAKO_ENABLE_ASAN "Enable AddressSanitizer (GCC/Clang, not MinGW GCC)" OFF)
option(EDITERAKO_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer (GCC/Clang)" OFF)
option(EDITERAKO_ENABLE_TSAN "Enable ThreadSanitizer (GCC/Clang, not Windows)" OFF)

function(editerako_apply_sanitizers)
    if(NOT EDITERAKO_ENABLE_ASAN AND NOT EDITERAKO_ENABLE_UBSAN AND NOT EDITERAKO_ENABLE_TSAN)
        return()
    endif()

    if(EDITERAKO_ENABLE_ASAN AND EDITERAKO_ENABLE_TSAN)
        message(FATAL_ERROR
            "ASan and TSan cannot be enabled together. "
            "Configure preset 'asan' or 'tsan', not both.")
    endif()

    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Sanitizers require GCC or Clang (compiler is ${CMAKE_CXX_COMPILER_ID}).")
    endif()

    if(EDITERAKO_ENABLE_TSAN AND WIN32)
        message(FATAL_ERROR "ThreadSanitizer is not supported on Windows. Use the tsan preset on Linux or macOS.")
    endif()

    if(MINGW AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR
            "ASan/UBSan are not supported with MinGW GCC. "
            "Use Clang or GCC on Linux/macOS (cmake --preset asan / tsan).")
    endif()

    set(_sanitizers)
    if(EDITERAKO_ENABLE_ASAN)
        list(APPEND _sanitizers address)
    endif()
    if(EDITERAKO_ENABLE_UBSAN)
        list(APPEND _sanitizers undefined)
    endif()
    if(EDITERAKO_ENABLE_TSAN)
        list(APPEND _sanitizers thread)
    endif()
    list(JOIN _sanitizers "," _sanitize_csv)

    message(STATUS "Editerako sanitizers: ${_sanitize_csv}")

    add_compile_options(
        "-fsanitize=${_sanitize_csv}"
        -fno-omit-frame-pointer
        -g
        -O1
    )
    add_link_options("-fsanitize=${_sanitize_csv}")

    if(EDITERAKO_ENABLE_UBSAN)
        add_compile_options(-fno-sanitize-recover=undefined)
        add_link_options(-fno-sanitize-recover=undefined)
    endif()
endfunction()

editerako_apply_sanitizers()
