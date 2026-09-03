qt_add_library(EditerakoScm STATIC)

target_sources(EditerakoScm PRIVATE
    GitCliProvider.cpp
    GitCliProvider.h
    GitParsers.cpp
    GitParsers.h
    ISourceControlProvider.h
    SourceControlTypes.h
)

target_include_directories(EditerakoScm PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(EditerakoScm PUBLIC Qt6::Core)
target_compile_features(EditerakoScm PUBLIC cxx_std_20)
editerako_enable_warnings(EditerakoScm)

