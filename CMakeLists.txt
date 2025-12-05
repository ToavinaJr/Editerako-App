cmake_minimum_required(VERSION 3.16)
project(Editerako VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ---- Qt ----
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

if(QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS PdfWidgets Network Sql)
endif()

# ---- Tree-sitter statique (SANS ICU) ----
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
    # Add /Zc:__cplusplus and /permissive- for MSVC to properly detect C++17 and fix Qt 6.10.0 compatibility
    add_compile_options(/Zc:__cplusplus /permissive-)
else()
    target_compile_options(tree_sitter PRIVATE -w -Wno-unused-parameter)
endif()

# ---- Sources du projet ----
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    codeeditor.cpp
    codeeditor.h
    finddialog.cpp
    finddialog.h
    syntaxhighlighter.cpp
    syntaxhighlighter.h
    terminal.cpp
    chatwidget.cpp
    chatwidget.h
)

# ---- Exécutable principal ----
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(Editerako
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        terminal.ui         # SEUL le .ui supplémentaire nécessaire
    )
else()
    add_executable(Editerako ${PROJECT_SOURCES})
endif()

# ---- Include directories ----
target_include_directories(Editerako PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}   # pour que ui_terminal.h trouve terminal.h
    tree-sitter/tree-sitter/lib/include
    tree-sitter/tree-sitter-cpp/src/tree_sitter
    tree-sitter/tree-sitter-html/src/tree_sitter
)

# ---- Link libraries ----
target_link_libraries(Editerako PRIVATE
    tree_sitter
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Sql
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(Editerako PRIVATE Qt6::PdfWidgets)
endif()

# ---- Bundle properties pour Mac (optionnel) ----
if(QT_VERSION_MAJOR VERSION_LESS 6.1)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Editerako)
endif()

set_target_properties(Editerako PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# ---- Install ----
include(GNUInstallDirs)
install(TARGETS Editerako
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- Finalize Qt executable ----
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Editerako)
endif()

# If Qt Creator requests `all_qmllint` but the project has no QML modules,
# provide a no-op placeholder target so the build step doesn't fail.
add_custom_target(all_qmllint
    COMMENT "No QML files detected — placeholder target to satisfy Qt Creator"
)
