cmake_minimum_required(VERSION 3.21)
project(Editerako VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Warnings.cmake")

# Qt 6. PdfWidgets is a separate Qt module (Qt PDF); install it if find_package fails.
find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    Network
    Sql
    Concurrent
    PdfWidgets
)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
endif()

# ---- Tree-sitter (vendored runtime + C++ / HTML grammars) ----
add_library(tree_sitter STATIC
    tree-sitter/tree-sitter/lib/src/lib.c
    tree-sitter/tree-sitter-cpp/src/parser.c
    tree-sitter/tree-sitter-cpp/src/scanner.c
    tree-sitter/tree-sitter-html/src/parser.c
    tree-sitter/tree-sitter-html/src/scanner.c
)

target_include_directories(tree_sitter PUBLIC
    tree-sitter/tree-sitter/lib/include
    tree-sitter/tree-sitter/lib/src
    tree-sitter/tree-sitter-cpp/src
    tree-sitter/tree-sitter-html/src
)

target_compile_definitions(tree_sitter PRIVATE TREE_SITTER_NO_ICU=1)

if(MSVC)
    target_compile_options(tree_sitter PRIVATE /W0)
else()
    target_compile_options(tree_sitter PRIVATE -w)
endif()

# ---- Application ----
qt_add_executable(Editerako
    MANUAL_FINALIZATION
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    codeeditor.cpp
    codeeditor.h
    finddialog.cpp
    finddialog.h
    gotolinedialog.cpp
    gotolinedialog.h
    terminal.cpp
    terminal.h
    terminal.ui
    chatwidget.cpp
    chatwidget.h
    src/core/Logging.cpp
    src/core/Logging.h
    src/core/AppSettings.cpp
    src/core/AppSettings.h
    src/core/ThemeManager.cpp
    src/core/ThemeManager.h
    src/editor/EditorDocument.cpp
    src/editor/EditorDocument.h
    src/editor/EditorManager.cpp
    src/editor/EditorManager.h
    src/syntax/LanguageRegistry.cpp
    src/syntax/LanguageRegistry.h
    src/syntax/TreeSitterDocument.cpp
    src/syntax/TreeSitterDocument.h
    src/syntax/SyntaxHighlighter.cpp
    src/syntax/SyntaxHighlighter.h
    src/project/Workspace.cpp
    src/project/Workspace.h
    src/project/FileExplorer.cpp
    src/project/FileExplorer.h
    src/terminal/TerminalProcess.cpp
    src/terminal/TerminalProcess.h
    src/ai/ChatMessage.h
    src/ai/AiProvider.cpp
    src/ai/AiProvider.h
    src/ai/GeminiProvider.cpp
    src/ai/GeminiProvider.h
    src/ai/ChatRepository.cpp
    src/ai/ChatRepository.h
    src/ai/ContextBuilder.cpp
    src/ai/ContextBuilder.h
    resources/resources.qrc
)

editerako_enable_warnings(Editerako)

target_include_directories(Editerako PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    tree-sitter/tree-sitter/lib/include
)

target_link_libraries(Editerako PRIVATE
    tree_sitter
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Concurrent
    Qt6::PdfWidgets
)

set_target_properties(Editerako PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

include(GNUInstallDirs)
install(TARGETS Editerako
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(Editerako)

# Qt Creator requests this target when no QML modules exist.
add_custom_target(all_qmllint
    COMMENT "No QML files detected — placeholder target to satisfy Qt Creator"
)
