# Application target. Paths are relative to this directory (src/).

set(editerako_sources
    main.cpp

    app/MainWindow.cpp
    app/MainWindow.h
    app/MainWindow.ui

    core/Logging.cpp
    core/Logging.h
    core/AppSettings.cpp
    core/AppSettings.h
    core/ThemeManager.cpp
    core/ThemeManager.h
    core/CommandRegistry.cpp
    core/CommandRegistry.h
    core/SessionStore.cpp
    core/SessionStore.h
    core/DropPaths.cpp
    core/DropPaths.h
    core/AtomicFile.cpp
    core/AtomicFile.h

    editor/CodeEditor.cpp
    editor/CodeEditor.h
    editor/EditorDocument.cpp
    editor/EditorDocument.h
    editor/EditorManager.cpp
    editor/EditorManager.h
    editor/FindReplaceDialog.cpp
    editor/FindReplaceDialog.h
    editor/GoToLineDialog.cpp
    editor/GoToLineDialog.h

    syntax/LanguageRegistry.cpp
    syntax/LanguageRegistry.h
    syntax/TreeSitterDocument.cpp
    syntax/TreeSitterDocument.h
    syntax/SyntaxHighlighter.cpp
    syntax/SyntaxHighlighter.h

    project/Workspace.cpp
    project/Workspace.h
    project/FileExplorer.cpp
    project/FileExplorer.h
    project/FileWatcher.cpp
    project/FileWatcher.h

    terminal/Terminal.cpp
    terminal/Terminal.h
    terminal/Terminal.ui
    terminal/TerminalInput.cpp
    terminal/TerminalInput.h
    terminal/TerminalProcess.cpp
    terminal/TerminalProcess.h
    terminal/TerminalPanel.cpp
    terminal/TerminalPanel.h
    terminal/CommandHistory.cpp
    terminal/CommandHistory.h
    terminal/CommandDiscovery.cpp
    terminal/CommandDiscovery.h
    terminal/CommandCompleter.cpp
    terminal/CommandCompleter.h

    ui/UiHelpers.cpp
    ui/UiHelpers.h

    ai/ChatMessage.h
    ai/AiProvider.cpp
    ai/AiProvider.h
    ai/GeminiProvider.cpp
    ai/GeminiProvider.h
    ai/ChatRepository.cpp
    ai/ChatRepository.h
    ai/ContextBuilder.cpp
    ai/ContextBuilder.h
    ai/ChatWidget.cpp
    ai/ChatWidget.h

    viewers/ImageViewer.cpp
    viewers/ImageViewer.h
    viewers/PdfViewer.cpp
    viewers/PdfViewer.h
    viewers/FileKind.cpp
    viewers/FileKind.h
    viewers/ViewerManager.cpp
    viewers/ViewerManager.h
)

qt_add_executable(Editerako ${editerako_sources})

editerako_enable_warnings(Editerako)

target_include_directories(Editerako PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Editerako PRIVATE
    tree_sitter
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Concurrent
    Qt6::PdfWidgets
)

qt_add_resources(Editerako editerako_themes
    PREFIX "/editerako"
    BASE "${PROJECT_SOURCE_DIR}/resources"
    FILES
        "${PROJECT_SOURCE_DIR}/resources/themes/dark.qss"
        "${PROJECT_SOURCE_DIR}/resources/themes/light.qss"
)

set_target_properties(Editerako PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "src" FILES ${editerako_sources})

install(TARGETS Editerako
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Editerako
    OUTPUT_SCRIPT editerako_deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${editerako_deploy_script})
