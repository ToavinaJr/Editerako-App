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

# ---- Qt PDF (Qt6 seulement) ----
if(QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS PdfWidgets)
endif()

# ---- Tree-sitter libraries ----
find_library(TREE_SITTER_LIB tree-sitter)
find_library(TREE_SITTER_CPP_LIB tree-sitter-cpp)
find_library(TREE_SITTER_HTML_LIB tree-sitter-html)

# ---- Fallback statique pour tree-sitter-cpp ----
if(NOT TREE_SITTER_CPP_LIB)
    add_library(tree-sitter-cpp STATIC
        tree-sitter/tree-sitter-cpp/src/parser.c
        tree-sitter/tree-sitter-cpp/src/scanner.c
    )
    target_include_directories(tree-sitter-cpp PUBLIC
        tree-sitter/tree-sitter-cpp/src
    )
    set(TREE_SITTER_CPP_LIB tree-sitter-cpp)
endif()

# ---- Fallback statique pour tree-sitter-html ----
if(NOT TREE_SITTER_HTML_LIB)
    add_library(tree-sitter-html STATIC
        tree-sitter/tree-sitter-html/src/parser.c
        tree-sitter/tree-sitter-html/src/scanner.c
    )
    target_include_directories(tree-sitter-html PUBLIC
        tree-sitter/tree-sitter-html/src
    )
    set(TREE_SITTER_HTML_LIB tree-sitter-html)
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
)

# ---- Exécutable principal ----
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(Editerako
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(Editerako ${PROJECT_SOURCES})
endif()

# ---- Include paths ----
target_include_directories(Editerako PRIVATE
    tree-sitter/tree-sitter-cpp/src/tree_sitter
    tree-sitter/tree-sitter-html/src/tree_sitter
)

# ---- Linking des librairies ----
target_link_libraries(Editerako PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    ${TREE_SITTER_LIB}
    ${TREE_SITTER_CPP_LIB}
    ${TREE_SITTER_HTML_LIB}
)

# ---- Qt PDF (Qt6) ----
if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(Editerako PRIVATE Qt6::PdfWidgets)
endif()

# ---- Propriétés bundle (macOS et Windows) ----
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

# ---- Installation ----
include(GNUInstallDirs)
install(TARGETS Editerako
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- Finalisation Qt6 ----
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Editerako)
endif()
