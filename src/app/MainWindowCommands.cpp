#include "app/MainWindow.h"
#include "ui_MainWindow.h"

#include "app/DebugSession.h"
#include "app/LspSession.h"
#include "core/AppSettings.h"
#include "core/CommandRegistry.h"
#include "core/KeybindingManager.h"
#include "editor/CodeEditor.h"
#include "editor/EditorManager.h"
#include "tasks/TaskManager.h"
#include "ui/BottomPanel.h"
#include "viewers/FileKind.h"

#include <QAction>
#include <QCheckBox>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QStatusBar>
#include <QStringList>
#include <QTabWidget>

void MainWindow::connectActions()
{
    m_commands = new CommandRegistry(this);

    auto bind = [this](const QString &id, QAction *action, void (MainWindow::*slot)()) {
        m_commands->add(id, action);
        connect(action, &QAction::triggered, this, slot);
    };

    bind(QStringLiteral("file.new"), ui->actionFile, &MainWindow::newFile);
    bind(QStringLiteral("file.newFolder"), ui->actionNew_Document, &MainWindow::newFolder);
    bind(QStringLiteral("file.open"), ui->actionOpen_File, &MainWindow::openFile);
    bind(QStringLiteral("file.openFolder"), ui->actionOpen_Folder, &MainWindow::openFolder);
    bind(QStringLiteral("file.save"), ui->actionSave, &MainWindow::saveCurrentDocument);
    bind(QStringLiteral("file.saveAs"), ui->actionSave_As, &MainWindow::saveCurrentDocumentAs);
    bind(QStringLiteral("file.saveAll"), ui->actionSave_All, &MainWindow::saveAllDocuments);
    bind(QStringLiteral("file.close"), ui->actionClose, &MainWindow::closeCurrentTab);
    bind(QStringLiteral("file.closeOthers"), ui->actionClose_Others, &MainWindow::closeOtherTabs);
    bind(QStringLiteral("file.closeAll"), ui->actionClose_All, &MainWindow::closeAllTabs);
    bind(QStringLiteral("edit.find"), ui->actionFindReplace, &MainWindow::onActionFindReplace);
    bind(QStringLiteral("edit.gotoLine"), ui->actionGoToLine, &MainWindow::onActionGoToLine);

    auto bindEditor = [this](const QString &id, const QString &text, void (CodeEditor::*method)()) {
        QAction *action = m_commands->create(id, text);
        connect(action, &QAction::triggered, this, [this, method]() {
            if (CodeEditor *editor = currentEditor()) {
                (editor->*method)();
            }
        });
        return action;
    };

    QAction *toggleLineComment = bindEditor(QStringLiteral("edit.toggleLineComment"),
                                            tr("Toggle Line Comment"),
                                            &CodeEditor::toggleLineComment);
    QAction *toggleBlockComment = bindEditor(QStringLiteral("edit.toggleBlockComment"),
                                             tr("Toggle Block Comment"),
                                             &CodeEditor::toggleBlockComment);
    QAction *indent = bindEditor(QStringLiteral("edit.indent"), tr("Indent"),
                                 &CodeEditor::indentSelection);
    QAction *outdent = bindEditor(QStringLiteral("edit.outdent"), tr("Outdent"),
                                  &CodeEditor::outdentSelection);
    QAction *duplicateLine = bindEditor(QStringLiteral("edit.duplicateLine"), tr("Duplicate Line"),
                                        &CodeEditor::duplicateLine);
    QAction *deleteLine = bindEditor(QStringLiteral("edit.deleteLine"), tr("Delete Line"),
                                     &CodeEditor::deleteLine);
    QAction *moveLineUp = bindEditor(QStringLiteral("edit.moveLineUp"), tr("Move Line Up"),
                                     &CodeEditor::moveLineUp);
    QAction *moveLineDown = bindEditor(QStringLiteral("edit.moveLineDown"), tr("Move Line Down"),
                                       &CodeEditor::moveLineDown);
    QAction *selectLine = bindEditor(QStringLiteral("edit.selectLine"), tr("Select Line"),
                                     &CodeEditor::selectLine);
    QAction *joinLines = bindEditor(QStringLiteral("edit.joinLines"), tr("Join Lines"),
                                    &CodeEditor::joinLines);
    QAction *sortLines = bindEditor(QStringLiteral("edit.sortLines"), tr("Sort Lines"),
                                    &CodeEditor::sortSelectedLines);
    QAction *trimWs = bindEditor(QStringLiteral("edit.trimTrailingWhitespace"),
                                 tr("Trim Trailing Whitespace"),
                                 &CodeEditor::trimTrailingWhitespace);
    QAction *toSpaces = bindEditor(QStringLiteral("edit.convertIndentationToSpaces"),
                                   tr("Convert Indentation to Spaces"),
                                   &CodeEditor::convertIndentationToSpaces);
    QAction *toTabs = bindEditor(QStringLiteral("edit.convertIndentationToTabs"),
                                 tr("Convert Indentation to Tabs"),
                                 &CodeEditor::convertIndentationToTabs);
    QAction *selectNext = bindEditor(QStringLiteral("edit.selectNextOccurrence"),
                                     tr("Select Next Occurrence"),
                                     &CodeEditor::selectNextOccurrence);
    QAction *selectAllOcc = bindEditor(QStringLiteral("edit.selectAllOccurrences"),
                                       tr("Select All Occurrences"),
                                       &CodeEditor::selectAllOccurrences);

    QAction *triggerSuggest = m_commands->create(QStringLiteral("editor.triggerSuggest"),
                                                 tr("Trigger Suggest"));
    connect(triggerSuggest, &QAction::triggered, this, [this]() {
        if (m_lspSession) {
            m_lspSession->triggerCompletion();
        }
    });
    QAction *gotoDefinition = m_commands->create(QStringLiteral("editor.gotoDefinition"),
                                                 tr("Go to Definition"));
    connect(gotoDefinition, &QAction::triggered, this, [this]() {
        if (m_lspSession) {
            m_lspSession->goToDefinition();
        }
    });
    QAction *findReferences = m_commands->create(QStringLiteral("editor.findReferences"),
                                                 tr("Find References"));
    connect(findReferences, &QAction::triggered, this, [this]() {
        if (m_lspSession) {
            m_lspSession->findReferences();
        }
    });
    QAction *renameSymbol = m_commands->create(QStringLiteral("editor.renameSymbol"),
                                               tr("Rename Symbol"));
    connect(renameSymbol, &QAction::triggered, this, [this]() {
        if (m_lspSession) {
            m_lspSession->renameSymbol();
        }
    });
    QAction *documentSymbols = m_commands->create(QStringLiteral("editor.documentSymbols"),
                                                  tr("Go to Symbol in Editor..."));
    connect(documentSymbols, &QAction::triggered, this, [this]() {
        if (m_lspSession) {
            m_lspSession->showDocumentSymbols();
        }
    });
    QAction *workspaceSymbols = m_commands->create(QStringLiteral("editor.workspaceSymbols"),
                                                   tr("Go to Symbol in Workspace..."));
    connect(workspaceSymbols, &QAction::triggered, this, [this]() {
        if (m_lspSession) {
            m_lspSession->showWorkspaceSymbols();
        }
    });
    QAction *showHover = m_commands->create(QStringLiteral("editor.showHover"),
                                            tr("Show Hover"));
    connect(showHover, &QAction::triggered, this, [this]() {
        if (m_lspSession) {
            m_lspSession->triggerHover();
        }
    });

    QAction *toggleTerminal = m_commands->create(
        QStringLiteral("view.terminal"),
        tr("Toggle Terminal"));
    connect(toggleTerminal, &QAction::triggered, this, &MainWindow::toggleTerminal);

    QAction *toggleProblems = m_commands->create(
        QStringLiteral("workbench.problems"),
        tr("Toggle Problems"));
    connect(toggleProblems, &QAction::triggered, this, &MainWindow::toggleProblems);

    QAction *toggleSourceControl = m_commands->create(
        QStringLiteral("workbench.sourceControl"), tr("Toggle Source Control"));
    connect(toggleSourceControl, &QAction::triggered, this, &MainWindow::toggleSourceControl);

    QAction *toggleTasks = m_commands->create(QStringLiteral("workbench.tasks"), tr("Toggle Tasks"));
    connect(toggleTasks, &QAction::triggered, this, &MainWindow::toggleTasks);
    QAction *toggleOutput = m_commands->create(QStringLiteral("workbench.output"),
                                               tr("Toggle Output"));
    connect(toggleOutput, &QAction::triggered, this, &MainWindow::toggleOutput);
    QAction *toggleDebug = m_commands->create(QStringLiteral("workbench.debug"),
                                              tr("Toggle Debug"));
    connect(toggleDebug, &QAction::triggered, this, &MainWindow::toggleDebug);
    QAction *buildTask = m_commands->create(QStringLiteral("workbench.build"),
                                            tr("Run Build Task"));
    connect(buildTask, &QAction::triggered, this, &MainWindow::runBuildTask);

    auto bindCMake = [this](const QString &id, const QString &text) {
        QAction *action = m_commands->create(id, text);
        connect(action, &QAction::triggered, this, [this, id]() {
            if (m_tasks) {
                m_tasks->run(id);
            }
        });
        return action;
    };
    QAction *cmakeConfigure = bindCMake(QStringLiteral("cmake.configure"), tr("CMake: Configure"));
    QAction *cmakeBuild = bindCMake(QStringLiteral("cmake.build"), tr("CMake: Build"));
    QAction *cmakeClean = bindCMake(QStringLiteral("cmake.clean"), tr("CMake: Clean"));
    QAction *cmakeTest = bindCMake(QStringLiteral("cmake.test"), tr("CMake: Test"));
    QAction *cmakeRun = bindCMake(QStringLiteral("cmake.run"), tr("CMake: Run"));

    QAction *compareWithDisk = m_commands->create(
        QStringLiteral("file.compareWithDisk"), tr("Compare with Disk"));
    connect(compareWithDisk, &QAction::triggered, this, &MainWindow::compareWithDisk);

    QAction *markdownPreview = m_commands->create(
        QStringLiteral("file.markdownPreview"), tr("Open Markdown Preview"));
    connect(markdownPreview, &QAction::triggered, this, &MainWindow::openMarkdownPreview);

    QAction *commandPalette = m_commands->create(
        QStringLiteral("workbench.commandPalette"),
        tr("Command Palette"));
    connect(commandPalette, &QAction::triggered, this, &MainWindow::openCommandPalette);

    QAction *quickOpen = m_commands->create(
        QStringLiteral("workbench.quickOpen"),
        tr("Quick Open"));
    connect(quickOpen, &QAction::triggered, this, &MainWindow::openQuickOpen);

    QAction *workspaceSearch = m_commands->create(
        QStringLiteral("workbench.search"),
        tr("Search in Workspace"));
    connect(workspaceSearch, &QAction::triggered, this, &MainWindow::openWorkspaceSearch);

    QAction *preferences = m_commands->create(
        QStringLiteral("preferences.open"),
        tr("Preferences..."));
    connect(preferences, &QAction::triggered, this, &MainWindow::openPreferences);

    QAction *zoomIn = m_commands->create(QStringLiteral("editor.zoomIn"), tr("Zoom In"));
    connect(zoomIn, &QAction::triggered, this, [this]() {
        if (m_editorManager) {
            m_editorManager->adjustFontSize(1);
        }
    });
    QAction *zoomOut = m_commands->create(QStringLiteral("editor.zoomOut"), tr("Zoom Out"));
    connect(zoomOut, &QAction::triggered, this, [this]() {
        if (m_editorManager) {
            m_editorManager->adjustFontSize(-1);
        }
    });
    QAction *zoomReset = m_commands->create(QStringLiteral("editor.zoomReset"),
                                            tr("Reset Zoom"));
    connect(zoomReset, &QAction::triggered, this, [this]() {
        if (m_editorManager) {
            m_editorManager->resetFontSize();
        }
    });

    QAction *splitRight = m_commands->create(QStringLiteral("workbench.splitEditorRight"),
                                             tr("Split Right"));
    connect(splitRight, &QAction::triggered, this, &MainWindow::splitEditorRight);
    QAction *splitDown = m_commands->create(QStringLiteral("workbench.splitEditorDown"),
                                            tr("Split Down"));
    connect(splitDown, &QAction::triggered, this, &MainWindow::splitEditorDown);
    QAction *moveEditor = m_commands->create(QStringLiteral("workbench.moveEditor"),
                                             tr("Move Editor"));
    connect(moveEditor, &QAction::triggered, this, &MainWindow::moveEditor);
    QAction *closeGroup = m_commands->create(QStringLiteral("workbench.closeEditorGroup"),
                                             tr("Close Editor Group"));
    connect(closeGroup, &QAction::triggered, this, &MainWindow::closeEditorGroup);

    QAction *debugStart = m_commands->create(QStringLiteral("debug.startContinue"),
                                             tr("Start Debugging"));
    connect(debugStart, &QAction::triggered, this, [this]() {
        if (m_debugSession) {
            m_debugSession->start();
        }
    });
    QAction *debugStop = m_commands->create(QStringLiteral("debug.stop"), tr("Stop Debugging"));
    connect(debugStop, &QAction::triggered, this, [this]() {
        if (m_debugSession) {
            m_debugSession->stop();
        }
    });
    QAction *debugPause = m_commands->create(QStringLiteral("debug.pause"), tr("Pause"));
    connect(debugPause, &QAction::triggered, this, [this]() {
        if (m_debugSession) {
            m_debugSession->pause();
        }
    });
    QAction *debugStepOver = m_commands->create(QStringLiteral("debug.stepOver"), tr("Step Over"));
    connect(debugStepOver, &QAction::triggered, this, [this]() {
        if (m_debugSession) {
            m_debugSession->stepOver();
        }
    });
    QAction *debugStepInto = m_commands->create(QStringLiteral("debug.stepInto"), tr("Step Into"));
    connect(debugStepInto, &QAction::triggered, this, [this]() {
        if (m_debugSession) {
            m_debugSession->stepInto();
        }
    });
    QAction *debugStepOut = m_commands->create(QStringLiteral("debug.stepOut"), tr("Step Out"));
    connect(debugStepOut, &QAction::triggered, this, [this]() {
        if (m_debugSession) {
            m_debugSession->stepOut();
        }
    });
    QAction *debugToggleBp = m_commands->create(QStringLiteral("debug.toggleBreakpoint"),
                                                tr("Toggle Breakpoint"));
    connect(debugToggleBp, &QAction::triggered, this, [this]() {
        if (m_debugSession) {
            m_debugSession->toggleBreakpointAtCursor();
        }
    });
    QAction *debugLaunchJson = m_commands->create(QStringLiteral("debug.createLaunchJson"),
                                                  tr("Create launch.json"));
    connect(debugLaunchJson, &QAction::triggered, this, [this]() {
        if (!m_debugSession) {
            return;
        }
        QString error;
        if (!m_debugSession->createLaunchFile(&error) && statusBar()) {
            statusBar()->showMessage(error, 4000);
        }
        if (m_bottomPanel) {
            m_bottomPanel->showDebug();
        }
    });

    m_keybindings = new KeybindingManager(m_commands, this);
    m_keybindings->apply();

    QMenu *editMenu = menuBar()->addMenu(tr("Edit"));
    editMenu->addAction(ui->actionFindReplace);
    editMenu->addAction(ui->actionGoToLine);
    editMenu->addSeparator();
    editMenu->addAction(toggleLineComment);
    editMenu->addAction(toggleBlockComment);
    editMenu->addSeparator();
    editMenu->addAction(indent);
    editMenu->addAction(outdent);
    editMenu->addAction(duplicateLine);
    editMenu->addAction(deleteLine);
    editMenu->addAction(moveLineUp);
    editMenu->addAction(moveLineDown);
    editMenu->addAction(selectLine);
    editMenu->addAction(joinLines);
    editMenu->addAction(sortLines);
    editMenu->addSeparator();
    editMenu->addAction(selectNext);
    editMenu->addAction(selectAllOcc);
    editMenu->addSeparator();
    editMenu->addAction(trimWs);
    editMenu->addAction(toSpaces);
    editMenu->addAction(toTabs);

    QMenu *goMenu = menuBar()->addMenu(tr("Go"));
    goMenu->addAction(gotoDefinition);
    goMenu->addAction(findReferences);
    goMenu->addAction(documentSymbols);
    goMenu->addAction(workspaceSymbols);
    goMenu->addSeparator();
    goMenu->addAction(triggerSuggest);
    goMenu->addAction(showHover);
    goMenu->addAction(renameSymbol);

    QMenu *viewMenu = menuBar()->addMenu(tr("View"));
    viewMenu->addAction(commandPalette);
    viewMenu->addAction(quickOpen);
    viewMenu->addAction(workspaceSearch);
    viewMenu->addSeparator();
    viewMenu->addAction(toggleProblems);
    viewMenu->addAction(toggleSourceControl);
    viewMenu->addAction(toggleTasks);
    viewMenu->addAction(toggleOutput);
    viewMenu->addAction(toggleDebug);
    viewMenu->addAction(toggleTerminal);
    viewMenu->addAction(compareWithDisk);
    viewMenu->addAction(markdownPreview);
    viewMenu->addSeparator();
    viewMenu->addAction(zoomIn);
    viewMenu->addAction(zoomOut);
    viewMenu->addAction(zoomReset);
    viewMenu->addSeparator();
    viewMenu->addAction(splitRight);
    viewMenu->addAction(splitDown);
    viewMenu->addAction(moveEditor);
    viewMenu->addAction(closeGroup);
    viewMenu->addSeparator();
    viewMenu->addAction(preferences);

    QMenu *buildMenu = menuBar()->addMenu(tr("Build"));
    buildMenu->addAction(buildTask);
    buildMenu->addSeparator();
    buildMenu->addAction(cmakeConfigure);
    buildMenu->addAction(cmakeBuild);
    buildMenu->addAction(cmakeClean);
    buildMenu->addAction(cmakeTest);
    buildMenu->addAction(cmakeRun);

    QMenu *debugMenu = menuBar()->addMenu(tr("Debug"));
    debugMenu->addAction(debugStart);
    debugMenu->addAction(debugStop);
    debugMenu->addAction(debugPause);
    debugMenu->addSeparator();
    debugMenu->addAction(debugStepOver);
    debugMenu->addAction(debugStepInto);
    debugMenu->addAction(debugStepOut);
    debugMenu->addSeparator();
    debugMenu->addAction(debugToggleBp);
    debugMenu->addAction(debugLaunchJson);
    debugMenu->addAction(toggleDebug);

    connect(ui->addFileButton, &QPushButton::clicked, this, &MainWindow::onAddFileClicked);
    connect(ui->newFolderButton, &QPushButton::clicked, this, &MainWindow::onNewFolderClicked);
    connect(ui->closeExplorerButton, &QPushButton::clicked, this, &MainWindow::onCloseExplorerClicked);
    connect(ui->checkBox, &QCheckBox::toggled, this, &MainWindow::onShowLinesToggled);
    ui->checkBox->setChecked(AppSettings().editorLineNumbers());

    updateCommandStates();
}

void MainWindow::updateCommandStates()
{
    if (!m_commands) {
        return;
    }

    const bool hasEditor = currentEditor() != nullptr;
    const int tabCount = m_editorManager ? m_editorManager->activeTabCount() : 0;
    const int totalTabs = m_editorManager ? m_editorManager->totalTabCount() : 0;
    const int groups = m_editorManager ? m_editorManager->groupCount() : 0;

    m_commands->setEnabled(QStringLiteral("file.save"), hasEditor);
    m_commands->setEnabled(QStringLiteral("file.saveAs"), hasEditor);
    m_commands->setEnabled(QStringLiteral("file.saveAll"), hasEditor);
    m_commands->setEnabled(QStringLiteral("edit.find"), hasEditor);
    m_commands->setEnabled(QStringLiteral("edit.gotoLine"), hasEditor);
    const QStringList editIds{
        QStringLiteral("edit.toggleLineComment"),
        QStringLiteral("edit.toggleBlockComment"),
        QStringLiteral("edit.indent"),
        QStringLiteral("edit.outdent"),
        QStringLiteral("edit.duplicateLine"),
        QStringLiteral("edit.deleteLine"),
        QStringLiteral("edit.moveLineUp"),
        QStringLiteral("edit.moveLineDown"),
        QStringLiteral("edit.selectLine"),
        QStringLiteral("edit.joinLines"),
        QStringLiteral("edit.sortLines"),
        QStringLiteral("edit.trimTrailingWhitespace"),
        QStringLiteral("edit.convertIndentationToSpaces"),
        QStringLiteral("edit.convertIndentationToTabs"),
        QStringLiteral("edit.selectNextOccurrence"),
        QStringLiteral("edit.selectAllOccurrences"),
    };
    for (const QString &id : editIds) {
        m_commands->setEnabled(id, hasEditor);
    }
    const QStringList lspIds{
        QStringLiteral("editor.triggerSuggest"),
        QStringLiteral("editor.gotoDefinition"),
        QStringLiteral("editor.findReferences"),
        QStringLiteral("editor.renameSymbol"),
        QStringLiteral("editor.documentSymbols"),
        QStringLiteral("editor.workspaceSymbols"),
        QStringLiteral("editor.showHover"),
    };
    for (const QString &id : lspIds) {
        m_commands->setEnabled(id, hasEditor);
    }
    const QStringList zoomIds{
        QStringLiteral("editor.zoomIn"),
        QStringLiteral("editor.zoomOut"),
        QStringLiteral("editor.zoomReset"),
    };
    for (const QString &id : zoomIds) {
        m_commands->setEnabled(id, hasEditor);
    }
    const bool hasSavedEditor = hasEditor && m_editorManager
        && !m_editorManager->currentFilePath().isEmpty();
    m_commands->setEnabled(QStringLiteral("file.compareWithDisk"), hasSavedEditor);
    m_commands->setEnabled(QStringLiteral("file.markdownPreview"),
                           isMarkdownPath(m_editorManager ? m_editorManager->currentFilePath()
                                                         : QString()));
    m_commands->setEnabled(QStringLiteral("file.close"), tabCount > 0);
    m_commands->setEnabled(QStringLiteral("file.closeOthers"), tabCount > 1);
    m_commands->setEnabled(QStringLiteral("file.closeAll"), totalTabs > 0);
    m_commands->setEnabled(QStringLiteral("workbench.splitEditorRight"), tabCount > 0);
    m_commands->setEnabled(QStringLiteral("workbench.splitEditorDown"), tabCount > 0);
    m_commands->setEnabled(QStringLiteral("workbench.moveEditor"), tabCount > 0);
    m_commands->setEnabled(QStringLiteral("workbench.closeEditorGroup"), groups > 1);

    const auto debugState = m_debugSession ? m_debugSession->state() : DebugSession::State::Idle;
    const bool debugActive = debugState == DebugSession::State::Starting
        || debugState == DebugSession::State::Running
        || debugState == DebugSession::State::Stopped;
    m_commands->setEnabled(QStringLiteral("debug.startContinue"),
                           debugState != DebugSession::State::Starting
                               && debugState != DebugSession::State::Running);
    m_commands->setEnabled(QStringLiteral("debug.stop"), debugActive);
    m_commands->setEnabled(QStringLiteral("debug.pause"), debugState == DebugSession::State::Running);
    m_commands->setEnabled(QStringLiteral("debug.stepOver"), debugState == DebugSession::State::Stopped);
    m_commands->setEnabled(QStringLiteral("debug.stepInto"), debugState == DebugSession::State::Stopped);
    m_commands->setEnabled(QStringLiteral("debug.stepOut"), debugState == DebugSession::State::Stopped);
    m_commands->setEnabled(QStringLiteral("debug.toggleBreakpoint"), hasEditor);
}
