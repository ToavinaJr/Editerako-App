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
endfunction()
