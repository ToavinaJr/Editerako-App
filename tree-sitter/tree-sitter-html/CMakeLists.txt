cmake_minimum_required(VERSION 3.16)

project(CodeEditor VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Trouver les packages Qt nécessaires
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Sources de l'application
set(SOURCES
    main.cpp
    mainwindow.cpp
    codeeditor.cpp
    filemanager.cpp
    finddialog.cpp
    simplehighlighter.cpp
)

# Headers de l'application
set(HEADERS
    mainwindow.h
    codeeditor.h
    filemanager.h
    finddialog.h
    simplehighlighter.h
)

# UI files
qt6_add_resources(RESOURCES mainwindow.ui)

# Créer l'exécutable
qt6_add_executable(Editerako ${SOURCES} ${HEADERS} mainwindow.ui)

# Lier les bibliothèques Qt
target_link_libraries(Editerako Qt6::Core Qt6::Widgets)

# Propriétés spécifiques pour Tree-sitter
target_compile_definitions(CodeEditor PRIVATE
    TREE_SITTER_HIDE_SYMBOLS
)

# Compiler les sources C avec les bons flags
set_source_files_properties(${TREE_SITTER_SOURCES} PROPERTIES
    LANGUAGE C
    COMPILE_FLAGS "-std=c99 -fPIC"
)

# Configuration pour Windows
if(WIN32)
    target_compile_definitions(CodeEditor PRIVATE WIN32_LEAN_AND_MEAN)
endif()

# Configuration pour les systèmes Unix
if(UNIX)
    target_compile_options(CodeEditor PRIVATE -fPIC)
endif()

# Installation
install(TARGETS CodeEditor
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
