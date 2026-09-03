include("${PROJECT_SOURCE_DIR}/cmake/Testing.cmake")

editerako_add_test(test_LanguageRegistry
    SOURCES LanguageRegistryTest.cpp
    LIBS EditerakoSyntax
)

editerako_add_test(test_Workspace
    SOURCES WorkspaceTest.cpp
    LIBS EditerakoProject
)

editerako_add_test(test_DropPaths
    SOURCES DropPathsTest.cpp
    LIBS EditerakoCore
)

editerako_add_test(test_SessionStore
    SOURCES SessionStoreTest.cpp
    LIBS EditerakoCore
)

editerako_add_test(test_ContextBuilder
    SOURCES ContextBuilderTest.cpp
    LIBS EditerakoAI
)

editerako_add_test(test_FileKind
    SOURCES FileKindTest.cpp
    LIBS EditerakoViewers
)

editerako_add_test(test_CommandRegistry
    SOURCES CommandRegistryTest.cpp
    LIBS EditerakoCore
)

set_tests_properties(test_CommandRegistry PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

editerako_add_test(test_AtomicFile
    SOURCES AtomicFileTest.cpp
    LIBS EditerakoCore
)

editerako_add_test(test_CommandHistory
    SOURCES CommandHistoryTest.cpp
    LIBS EditerakoTerminal
)

editerako_add_test(test_CommandCompleter
    SOURCES CommandCompleterTest.cpp
    LIBS EditerakoTerminal
)

editerako_add_test(test_TreeSitterDocument
    SOURCES TreeSitterDocumentTest.cpp
    LIBS EditerakoSyntax
)

set_tests_properties(test_TreeSitterDocument PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

editerako_add_test(test_HighlightQuery
    SOURCES HighlightQueryTest.cpp
    LIBS EditerakoSyntax
)

target_compile_definitions(test_HighlightQuery PRIVATE
    EDITERAKO_QUERY_DIR="${PROJECT_SOURCE_DIR}/resources/syntax"
)
