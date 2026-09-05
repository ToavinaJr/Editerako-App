include("${PROJECT_SOURCE_DIR}/cmake/Testing.cmake")

set_directory_properties(PROPERTIES QT_EXCLUDE_FROM_TRANSLATION ON)

editerako_add_test(test_LanguageRegistry
    SOURCES LanguageRegistryTest.cpp
    LIBS EditerakoSyntax
)

editerako_add_test(test_Workspace
    SOURCES WorkspaceTest.cpp
    LIBS EditerakoProject
)

editerako_add_test(test_FileExplorerDecorations
    SOURCES FileExplorerDecorationsTest.cpp
    LIBS EditerakoProject
)

editerako_add_test(test_WorkspaceController
    SOURCES WorkspaceControllerTest.cpp
    LIBS EditerakoProject
)

editerako_test_offscreen(test_WorkspaceController)

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

editerako_add_test(test_RecentWorkspaces
    SOURCES RecentWorkspacesTest.cpp
    LIBS EditerakoCore
)

editerako_add_test(test_ContextBuilder
    SOURCES ContextBuilderTest.cpp
    LIBS EditerakoAI
)

editerako_add_test(test_AiCatalog
    SOURCES AiCatalogTest.cpp
    LIBS EditerakoAI
)

editerako_add_test(test_AiResponseParse
    SOURCES AiResponseParseTest.cpp
    LIBS EditerakoAI
)

editerako_add_test(test_FileKind
    SOURCES FileKindTest.cpp
    LIBS EditerakoViewers
)

editerako_add_test(test_CsvParser
    SOURCES CsvParserTest.cpp
    LIBS EditerakoViewers
)

editerako_add_test(test_BuiltinViewers
    SOURCES BuiltinViewersTest.cpp
    LIBS EditerakoViewers
)

editerako_test_offscreen(test_BuiltinViewers)

editerako_add_test(test_CommandRegistry
    SOURCES CommandRegistryTest.cpp
    LIBS EditerakoCore
)

editerako_test_offscreen(test_CommandRegistry)

editerako_add_test(test_AtomicFile
    SOURCES AtomicFileTest.cpp
    LIBS EditerakoCore
)

editerako_add_test(test_BackupService
    SOURCES BackupServiceTest.cpp
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

editerako_add_test(test_TranslationLoader
    SOURCES TranslationLoaderTest.cpp
    LIBS EditerakoCore
)

target_compile_definitions(test_TranslationLoader PRIVATE
    EDITERAKO_TRANSLATIONS_DIR="${PROJECT_SOURCE_DIR}/translations"
)

editerako_add_test(test_KeybindingModel
    SOURCES KeybindingModelTest.cpp
    LIBS EditerakoCore
)

editerako_test_offscreen(test_KeybindingModel)

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

editerako_add_test(test_AnsiSgr
    SOURCES AnsiSgrTest.cpp
    LIBS EditerakoTerminal
)

editerako_add_test(test_ShellProfiles
    SOURCES ShellProfilesTest.cpp
    LIBS EditerakoTerminal
)

editerako_add_test(test_ProcessTerminalBackend
    SOURCES ProcessTerminalBackendTest.cpp
    LIBS EditerakoTerminal
)

editerako_add_test(test_PtyTerminalBackend
    SOURCES PtyTerminalBackendTest.cpp
    LIBS EditerakoTerminal
)

editerako_add_test(test_TreeSitterDocument
    SOURCES TreeSitterDocumentTest.cpp
    LIBS EditerakoSyntax
)

editerako_test_offscreen(test_TreeSitterDocument)

editerako_add_test(test_FoldRanges
    SOURCES FoldRangesTest.cpp
    LIBS EditerakoSyntax
)

editerako_test_offscreen(test_FoldRanges)

editerako_add_test(test_FoldingController
    SOURCES FoldingControllerTest.cpp
    LIBS EditerakoEditor
)

editerako_test_offscreen(test_FoldingController)

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

editerako_test_offscreen(test_LineMovementController)

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

editerako_test_offscreen(test_LineEditCommands)

editerako_add_test(test_MultiCursorController
    SOURCES MultiCursorControllerTest.cpp
    LIBS EditerakoEditor
)

editerako_test_offscreen(test_MultiCursorController)

editerako_add_test(test_EditorDocument
    SOURCES EditorDocumentTest.cpp
    LIBS EditerakoEditor
)

editerako_test_offscreen(test_EditorDocument)

editerako_add_test(test_EditorArea
    SOURCES EditorAreaTest.cpp
    LIBS EditerakoEditor
)

editerako_test_offscreen(test_EditorArea)

editerako_add_test(test_TabOps
    SOURCES TabOpsTest.cpp
    LIBS EditerakoEditor
)

editerako_add_test(test_StatusBarText
    SOURCES StatusBarTextTest.cpp
    LIBS EditerakoEditor
)

editerako_add_test(test_EditorGroup
    SOURCES EditorGroupTest.cpp
    LIBS EditerakoEditor
)

editerako_test_offscreen(test_EditorGroup)

editerako_add_test(test_LspMessageFramer
    SOURCES LspMessageFramerTest.cpp
    LIBS EditerakoLsp
)

editerako_add_test(test_LspTypes
    SOURCES LspTypesTest.cpp
    LIBS EditerakoLsp
)

editerako_add_test(test_LspPickerItems
    SOURCES LspPickerItemsTest.cpp
    LIBS EditerakoLsp
)

editerako_add_test(test_LspCompileCommands
    SOURCES LspCompileCommandsTest.cpp
    LIBS EditerakoLsp
)

editerako_add_test(test_LspHoverHtml
    SOURCES LspHoverHtmlTest.cpp
    LIBS EditerakoLsp
)

editerako_add_test(test_LspClient
    SOURCES LspClientTest.cpp
    LIBS EditerakoLsp
)

editerako_add_test(test_LspServerProcess
    SOURCES LspServerProcessTest.cpp
    LIBS EditerakoLsp
)

editerako_add_test(test_DapClient
    SOURCES DapClientTest.cpp
    LIBS EditerakoDap
)

editerako_add_test(test_LaunchFile
    SOURCES LaunchFileTest.cpp
    LIBS EditerakoDap
)

editerako_add_test(test_BreakpointStore
    SOURCES BreakpointStoreTest.cpp
    LIBS EditerakoDap
)

editerako_add_test(test_DapTypes
    SOURCES DapTypesTest.cpp
    LIBS EditerakoDap
)

editerako_add_test(test_CompletionModel
    SOURCES CompletionModelTest.cpp
    LIBS EditerakoEditor
)

editerako_test_offscreen(test_CompletionModel)

editerako_add_test(test_DiagnosticMarkup
    SOURCES DiagnosticMarkupTest.cpp
    LIBS EditerakoEditor
)

editerako_test_offscreen(test_DiagnosticMarkup)

editerako_add_test(test_ProblemModel
    SOURCES ProblemModelTest.cpp
    LIBS EditerakoEditor
)

editerako_add_test(test_GitParsers
    SOURCES GitParsersTest.cpp
    LIBS EditerakoScm
)

editerako_add_test(test_TextDiff
    SOURCES TextDiffTest.cpp
    LIBS EditerakoScm
)

editerako_add_test(test_GitCliProvider
    SOURCES GitCliProviderTest.cpp
    LIBS EditerakoScm
)

editerako_add_test(test_TaskFile
    SOURCES TaskFileTest.cpp
    LIBS EditerakoTasks
)

editerako_add_test(test_CMakePresets
    SOURCES CMakePresetsTest.cpp
    LIBS EditerakoTasks
)

editerako_add_test(test_ProblemMatcher
    SOURCES ProblemMatcherTest.cpp
    LIBS EditerakoTasks
)

editerako_add_test(test_TaskManager
    SOURCES TaskManagerTest.cpp
    LIBS EditerakoTasks
)

editerako_add_test(test_PluginManifest
    SOURCES PluginManifestTest.cpp
    LIBS EditerakoPlugins
)

editerako_add_test(test_PluginManager
    SOURCES PluginManagerTest.cpp
    LIBS EditerakoPlugins EditerakoViewers
)

editerako_test_offscreen(test_PluginManager)

