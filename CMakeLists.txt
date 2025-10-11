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
    find_package(Qt6 REQUIRED COMPONENTS PdfWidgets)
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

# Définir les flags de compilation pour désactiver ICU
target_compile_definitions(tree_sitter PRIVATE
    TREE_SITTER_NO_ICU=1
)

# Désactiver les warnings
if(MSVC)
    target_compile_options(tree_sitter PRIVATE /W0)
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

target_include_directories(Editerako PRIVATE
    tree-sitter/tree-sitter/lib/include
    tree-sitter/tree-sitter-cpp/src/tree_sitter
    tree-sitter/tree-sitter-html/src/tree_sitter
)

target_link_libraries(Editerako PRIVATE
    tree_sitter
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(Editerako PRIVATE Qt6::PdfWidgets)
endif()

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

include(GNUInstallDirs)
install(TARGETS Editerako
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Editerako)
endif()
