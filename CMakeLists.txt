cmake_minimum_required(VERSION 3.16)
project(Editerako VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Qt ----
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# ---- Tree-sitter libraries ----
# Utiliser les librairies système
find_library(TREE_SITTER_LIB tree-sitter REQUIRED)
find_library(TREE_SITTER_CPP_LIB tree-sitter-cpp REQUIRED)

# Si tree-sitter-html n'est pas installée système, utiliser nos sources
find_library(TREE_SITTER_HTML_LIB tree-sitter-html)
if(NOT TREE_SITTER_HTML_LIB)
    # Créer tree-sitter-html depuis nos sources
    add_library(tree-sitter-html STATIC
        tree-sitter/tree-sitter-html/src/parser.c
        tree-sitter/tree-sitter-html/src/scanner.c
    )
    target_include_directories(tree-sitter-html PUBLIC
        tree-sitter/tree-sitter-html/src
    )
    set(TREE_SITTER_HTML_LIB tree-sitter-html)
endif()

# ---- Sources du projet principal ----
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


)

# ---- Exécutable principal ----
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Editerako
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(Editerako ${PROJECT_SOURCES})
endif()

# ---- Configuration des includes ----
target_include_directories(Editerako PRIVATE
    /usr/include/tree_sitter
    /usr/local/include/tree_sitter
    tree-sitter/tree-sitter-cpp/src/tree_sitter
    tree-sitter/tree-sitter-html/src/tree_sitter
    tree-sitter/tree-sitter-cpp/src
    tree-sitter/tree-sitter-html/src
)

# ---- Linking des librairies ----
target_link_libraries(Editerako PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    ${TREE_SITTER_LIB}
    ${TREE_SITTER_CPP_LIB}
    ${TREE_SITTER_HTML_LIB}
)

# ---- Propriétés bundle ----
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Editerako)
endif()

set_target_properties(Editerako PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# ---- Installation ----
include(GNUInstallDirs)
install(TARGETS Editerako
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Editerako)
endif()
