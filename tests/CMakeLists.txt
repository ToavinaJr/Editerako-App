include("${PROJECT_SOURCE_DIR}/cmake/Testing.cmake")

editerako_add_test(test_LanguageRegistry
    SOURCES LanguageRegistryTest.cpp
            "${PROJECT_SOURCE_DIR}/src/syntax/LanguageRegistry.cpp"
    LIBS tree_sitter
)

editerako_add_test(test_Workspace
    SOURCES WorkspaceTest.cpp
            "${PROJECT_SOURCE_DIR}/src/project/Workspace.cpp"
            "${PROJECT_SOURCE_DIR}/src/core/AppSettings.cpp"
            "${PROJECT_SOURCE_DIR}/src/core/Logging.cpp"
    LIBS Qt6::Core
)

editerako_add_test(test_DropPaths
    SOURCES DropPathsTest.cpp
            "${PROJECT_SOURCE_DIR}/src/core/DropPaths.cpp"
    LIBS Qt6::Core
)

editerako_add_test(test_SessionStore
    SOURCES SessionStoreTest.cpp
            "${PROJECT_SOURCE_DIR}/src/core/SessionStore.cpp"
            "${PROJECT_SOURCE_DIR}/src/core/Logging.cpp"
    LIBS Qt6::Core
)

editerako_add_test(test_ContextBuilder
    SOURCES ContextBuilderTest.cpp
            "${PROJECT_SOURCE_DIR}/src/ai/ContextBuilder.cpp"
    LIBS Qt6::Core
)

editerako_add_test(test_FileKind
    SOURCES FileKindTest.cpp
            "${PROJECT_SOURCE_DIR}/src/viewers/FileKind.cpp"
    LIBS Qt6::Core
)

editerako_add_test(test_CommandRegistry
    SOURCES CommandRegistryTest.cpp
            "${PROJECT_SOURCE_DIR}/src/core/CommandRegistry.cpp"
            "${PROJECT_SOURCE_DIR}/src/core/Logging.cpp"
    LIBS Qt6::Widgets
)

set_tests_properties(test_CommandRegistry PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
