include("${PROJECT_SOURCE_DIR}/cmake/Testing.cmake")

editerako_add_test(test_LanguageRegistry
    SOURCES LanguageRegistryTest.cpp
    LIBS EditerakoSyntax
)

editerako_add_test(test_Workspace
    SOURCES WorkspaceTest.cpp
    LIBS EditerakoProject
)

editerako_add_test(test_WorkspaceController
    SOURCES WorkspaceControllerTest.cpp
    LIBS EditerakoProject
)

set_tests_properties(test_WorkspaceController PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

editerako_add_test(test_SessionController
    SOURCES SessionControllerTest.cpp
    LIBS EditerakoCore
)

editerako_add_test(test_DiskChangePolicy
    SOURCES DiskChangePolicyTest.cpp
    LIBS EditerakoCore
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

editerako_add_test(test_TextFileFormat
    SOURCES TextFileFormatTest.cpp
    LIBS EditerakoCore
)

editerako_add_test(test_AppSettings
    SOURCES AppSettingsTest.cpp
    LIBS EditerakoCore
)

editerako_add_test(test_KeybindingModel
    SOURCES KeybindingModelTest.cpp
    LIBS EditerakoCore
)

set_tests_properties(test_KeybindingModel PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

editerako_add_test(test_FuzzyMatcher
    SOURCES FuzzyMatcherTest.cpp
    LIBS EditerakoCore
)

editerako_add_test(test_WorkspaceFileIndex
    SOURCES WorkspaceFileIndexTest.cpp
    LIBS EditerakoProject
)

editerako_add_test(test_WorkspacePath
    SOURCES WorkspacePathTest.cpp
    LIBS EditerakoProject
)

editerako_add_test(test_GitIgnore
    SOURCES GitIgnoreTest.cpp
    LIBS EditerakoProject
)

editerako_add_test(test_WorkspaceSearch
    SOURCES WorkspaceSearchTest.cpp
    LIBS EditerakoProject
)

editerako_add_test(test_WorkspaceOps
    SOURCES WorkspaceOpsTest.cpp
    LIBS EditerakoProject
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

editerako_add_test(test_EditorIo
    SOURCES EditorIoTest.cpp
    LIBS EditerakoEditor
)

editerako_add_test(test_HighlighterSync
    SOURCES HighlighterSyncTest.cpp
    LIBS EditerakoEditor
)

editerako_add_test(test_LineMovementController
    SOURCES LineMovementControllerTest.cpp
    LIBS EditerakoEditor
)

set_tests_properties(test_LineMovementController PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

editerako_add_test(test_IndentOps
    SOURCES IndentOpsTest.cpp
    LIBS EditerakoEditor
)

editerako_add_test(test_CommentOps
    SOURCES CommentOpsTest.cpp
    LIBS EditerakoEditor
)

editerako_add_test(test_BracketMatcher
    SOURCES BracketMatcherTest.cpp
    LIBS EditerakoEditor
)

editerako_add_test(test_LineEditCommands
    SOURCES LineEditCommandsTest.cpp
    LIBS EditerakoEditor
)

set_tests_properties(test_LineEditCommands PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

editerako_add_test(test_MultiCursorController
    SOURCES MultiCursorControllerTest.cpp
    LIBS EditerakoEditor
)

set_tests_properties(test_MultiCursorController PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

editerako_add_test(test_EditorDocument
    SOURCES EditorDocumentTest.cpp
    LIBS EditerakoEditor
)

set_tests_properties(test_EditorDocument PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
