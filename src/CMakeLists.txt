# Application executable. Module libraries live in sibling subdirectories.

add_subdirectory(core)
add_subdirectory(syntax)
add_subdirectory(editor)
add_subdirectory(project)
add_subdirectory(terminal)
add_subdirectory(viewers)
add_subdirectory(ai)
add_subdirectory(lsp)

set(editerako_app_sources
    main.cpp
    app/MainWindow.cpp
    app/MainWindow.h
    app/MainWindow.ui
    ui/UiHelpers.cpp
    ui/UiHelpers.h
    ui/KeybindingEditor.cpp
    ui/KeybindingEditor.h
    ui/SettingsDialog.cpp
    ui/SettingsDialog.h
    ui/FuzzyPickerDialog.cpp
    ui/FuzzyPickerDialog.h
    ui/CommandPaletteDialog.cpp
    ui/CommandPaletteDialog.h
    ui/QuickOpenDialog.cpp
    ui/QuickOpenDialog.h
    ui/WorkspaceSearchDialog.cpp
    ui/WorkspaceSearchDialog.h
)

qt_add_executable(Editerako ${editerako_app_sources})

editerako_enable_warnings(Editerako)
target_compile_features(Editerako PRIVATE cxx_std_20)

target_include_directories(Editerako PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Editerako PRIVATE
    EditerakoCore
    EditerakoSyntax
    EditerakoEditor
    EditerakoProject
    EditerakoTerminal
    EditerakoViewers
    EditerakoAI
    EditerakoLsp
)

qt_add_resources(Editerako editerako_themes
    PREFIX "/editerako"
    BASE "${PROJECT_SOURCE_DIR}/resources"
    FILES
        "${PROJECT_SOURCE_DIR}/resources/themes/dark.qss"
        "${PROJECT_SOURCE_DIR}/resources/themes/light.qss"
)

qt_add_resources(Editerako editerako_syntax
    PREFIX "/editerako"
    BASE "${PROJECT_SOURCE_DIR}/resources"
    FILES
        "${PROJECT_SOURCE_DIR}/resources/syntax/c/highlights.scm"
        "${PROJECT_SOURCE_DIR}/resources/syntax/cpp/highlights.scm"
        "${PROJECT_SOURCE_DIR}/resources/syntax/cmake/highlights.scm"
        "${PROJECT_SOURCE_DIR}/resources/syntax/html/highlights.scm"
        "${PROJECT_SOURCE_DIR}/resources/syntax/css/highlights.scm"
        "${PROJECT_SOURCE_DIR}/resources/syntax/javascript/highlights.scm"
        "${PROJECT_SOURCE_DIR}/resources/syntax/typescript/highlights.scm"
        "${PROJECT_SOURCE_DIR}/resources/syntax/tsx/highlights.scm"
        "${PROJECT_SOURCE_DIR}/resources/syntax/json/highlights.scm"
        "${PROJECT_SOURCE_DIR}/resources/syntax/markdown/highlights.scm"
        "${PROJECT_SOURCE_DIR}/resources/syntax/python/highlights.scm"
        "${PROJECT_SOURCE_DIR}/resources/syntax/bash/highlights.scm"
        "${PROJECT_SOURCE_DIR}/resources/syntax/sql/highlights.scm"
        "${PROJECT_SOURCE_DIR}/resources/syntax/yaml/highlights.scm"
)

set_target_properties(Editerako PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "src" FILES ${editerako_app_sources})

install(TARGETS Editerako
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Editerako
    OUTPUT_SCRIPT editerako_deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${editerako_deploy_script})
