# Static Qt module libraries. PUBLIC usage requirements make test and app
# link lines explicit without recompiling the same .cpp files.

function(editerako_add_module name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "editerako_add_module(${name}): SOURCES is required")
    endif()

    qt_add_library(${name} STATIC)
    target_sources(${name} PRIVATE ${ARG_SOURCES})
    editerako_enable_warnings(${name})

    target_include_directories(${name} PUBLIC "${PROJECT_SOURCE_DIR}/src")
    target_compile_features(${name} PUBLIC cxx_std_20)

    if(ARG_LIBS)
        target_link_libraries(${name} PUBLIC ${ARG_LIBS})
    endif()

    set_target_properties(${name} PROPERTIES
        FOLDER "Modules"
    )

    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "${name}" FILES ${ARG_SOURCES})
endfunction()
