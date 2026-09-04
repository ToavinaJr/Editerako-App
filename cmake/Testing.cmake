function(editerako_add_test_environment name)
    if(EDITERAKO_ENABLE_ASAN)
        file(TO_CMAKE_PATH
            "${PROJECT_SOURCE_DIR}/cmake/sanitizer-suppressions/lsan.supp" _lsan)
        set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT_MODIFICATION
            "ASAN_OPTIONS=set:detect_leaks=1:abort_on_error=1:halt_on_error=1")
        set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT_MODIFICATION
            "LSAN_OPTIONS=set:suppressions=${_lsan}:print_suppressions=0")
    endif()
    if(EDITERAKO_ENABLE_UBSAN)
        set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT_MODIFICATION
            "UBSAN_OPTIONS=set:halt_on_error=1:print_stacktrace=1:print_summary=1")
    endif()
    if(EDITERAKO_ENABLE_TSAN)
        file(TO_CMAKE_PATH
            "${PROJECT_SOURCE_DIR}/cmake/sanitizer-suppressions/tsan.supp" _tsan)
        set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT_MODIFICATION
            "TSAN_OPTIONS=set:halt_on_error=1:second_deadlock_stack=1:suppressions=${_tsan}")
    endif()
endfunction()

function(editerako_test_offscreen name)
    set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT_MODIFICATION
        "QT_QPA_PLATFORM=set:offscreen")
endfunction()

function(editerako_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})

    qt_add_executable(${name} ${ARG_SOURCES})
    editerako_enable_warnings(${name})
    target_compile_features(${name} PRIVATE cxx_std_20)
    target_include_directories(${name} PRIVATE "${PROJECT_SOURCE_DIR}/src")
    target_link_libraries(${name} PRIVATE Qt6::Test ${ARG_LIBS})
    set_target_properties(${name} PROPERTIES
        WIN32_EXECUTABLE OFF
        MACOSX_BUNDLE OFF
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
        FOLDER "Tests"
    )
    add_test(NAME ${name} COMMAND ${name})
    editerako_add_test_environment(${name})
endfunction()
