cmake_minimum_required(VERSION 3.21)
project(Editerako VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Warnings.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Sanitizers.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Libraries.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/WebView2.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/TreeSitter.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Apple.cmake")

# Qt 6. PdfWidgets and SvgWidgets are separate Qt modules; install them if find_package fails.
find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    Network
    Sql
    Concurrent
    PdfWidgets
    Svg
    SvgWidgets
    Test
    LinguistTools
)

editerako_strip_agl_from_qt()

qt_standard_project_setup()

# Scripts and presets expect the binary at ${binaryDir}/Editerako(.exe),
# not in a subdirectory created by add_subdirectory(src).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

enable_testing()
include(GNUInstallDirs)
add_subdirectory(src)
add_subdirectory(tests)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Packaging.cmake")

# Qt Creator requests this target when no QML modules exist.
add_custom_target(all_qmllint
    COMMENT "No QML files detected — placeholder target to satisfy Qt Creator"
)
