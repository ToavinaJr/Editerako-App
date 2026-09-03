#include "app/MainWindow.h"
#include "ui_MainWindow.h"
#include "ai/ChatWidget.h"
#include "core/AppSettings.h"
#include "core/CommandRegistry.h"
#include "core/DiskChangePolicy.h"
#include "core/DropPaths.h"
#include "core/KeybindingManager.h"
#include "core/ThemeManager.h"
#include "editor/CodeEditor.h"
#include "editor/EditorDocument.h"
#include "editor/EditorManager.h"
#include "editor/EditorStatusWidget.h"
#include "editor/FindReplaceDialog.h"
#include "editor/GoToLineDialog.h"
#include "lsp/LspServerManager.h"
#include "app/LspSession.h"
#include "project/FileExplorer.h"
#include "project/WorkspaceController.h"
#include "terminal/TerminalPanel.h"
#include "ui/CommandPaletteDialog.h"
#include "ui/QuickOpenDialog.h"
#include "ui/SettingsDialog.h"
#include "ui/WorkspaceSearchDialog.h"
#include "ui/UiHelpers.h"
#include "viewers/ViewerManager.h"

#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStringList>
#include <QTabWidget>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , isFileTreeVisible(true)
{
    ui->setupUi(this);
    ui->centralStack->setCurrentIndex(CodeViewer);

    setupFileTree();
    setupCodeEditor();

    m_lsp = new LspServerManager(this);
    m_lspSession = new LspSession(m_lsp, m_editorManager, this);
    connect(m_lspSession, &LspSession::statusMessage, this, [this](const QString &message, int timeoutMs) {
        if (statusBar()) {
            statusBar()->showMessage(message, timeoutMs);
        }
    });
    connect(m_lspSession, &LspSession::lspStatusChanged, this, [this](const QString &text) {
        if (m_editorStatus) {
            m_editorStatus->setLspStatus(text);
        }
    });

    connectActions();
    restartAutoSave();
    updateWindowTitle();

    if (currentEditor()) {
        currentEditor()->setFocus();
    }

    setupTerminalPanel();
    installChatWidget();
    connectWorkspaceCollaborators();

    setAcceptDrops(true);

    if (!restoreSession()) {
        promptOpenFolderOrFile();
    }
    syncChatContext();
}

MainWindow::~MainWindow()
{
    if (m_lsp) {
        m_lsp->stopAll();
    }
    if (m_terminalPanel) {
        m_terminalPanel->shutdownAll();
    }
    delete ui;
}

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
    viewMenu->addAction(toggleTerminal);
    viewMenu->addSeparator();
    viewMenu->addAction(preferences);

    connect(ui->addFileButton, &QPushButton::clicked, this, &MainWindow::onAddFileClicked);
    connect(ui->newFolderButton, &QPushButton::clicked, this, &MainWindow::onNewFolderClicked);
    connect(ui->closeExplorerButton, &QPushButton::clicked, this, &MainWindow::onCloseExplorerClicked);
    connect(ui->checkBox, &QCheckBox::toggled, this, &MainWindow::onShowLinesToggled);
    ui->checkBox->setChecked(AppSettings().editorLineNumbers());

    updateCommandStates();
}

void MainWindow::connectWorkspaceCollaborators()
{
    connect(m_workspaceController, &WorkspaceController::rootPathChanged, this,
            [this](const QString &path) {
                AppSettings::setWorkspaceRoot(path);
                if (m_editorManager) {
                    m_editorManager->setWorkingDirectory(path);
                }
                if (m_lspSession) {
                    m_lspSession->setWorkspaceRoot(path);
                }
                if (m_terminalPanel) {
                    m_terminalPanel->setWorkingDirectory(path);
                }
                if (chatWidget) {
                    chatWidget->setProjectDirectory(path);
                }
                saveSession();
                updateWindowTitle();
            });
}

void MainWindow::updateCommandStates()
{
    if (!m_commands) {
        return;
    }

    const bool hasEditor = currentEditor() != nullptr;
    const int tabCount = m_editorManager ? m_editorManager->tabWidget()->count() : 0;

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
    m_commands->setEnabled(QStringLiteral("file.close"), tabCount > 0);
    m_commands->setEnabled(QStringLiteral("file.closeOthers"), tabCount > 1);
    m_commands->setEnabled(QStringLiteral("file.closeAll"), tabCount > 0);
}

void MainWindow::setupCodeEditor()
{
    m_editorManager = new EditorManager(this);

    QWidget *oldEditor = ui->centralStack->widget(CodeViewer);
    if (oldEditor) {
        ui->centralStack->removeWidget(oldEditor);
        oldEditor->deleteLater();
    }

    ui->centralStack->insertWidget(CodeViewer, m_editorManager->tabWidget());
    ui->centralStack->setCurrentIndex(CodeViewer);

    m_viewerManager = new ViewerManager(m_editorManager, this);

    connect(m_editorManager, &EditorManager::currentChanged, this, &MainWindow::updateWindowTitle);
    connect(m_editorManager, &EditorManager::currentChanged, this, &MainWindow::syncChatContext);
    connect(m_editorManager, &EditorManager::currentChanged, this, &MainWindow::updateCommandStates);
    connect(m_editorManager, &EditorManager::currentChanged, this, &MainWindow::saveSession);
    connect(m_editorManager, &EditorManager::currentChanged, this, &MainWindow::syncFileWatches);
    connect(m_editorManager, &EditorManager::modificationChanged, this, &MainWindow::updateWindowTitle);
    connect(m_editorManager, &EditorManager::aboutToSave, this, [this](const QString &path) {
        if (m_workspaceController) {
            m_workspaceController->ignoreNextChange(path);
        }
    });
    connect(m_editorManager, &EditorManager::fileSaved, this, [this](const QString &path) {
        if (m_workspaceController) {
            m_workspaceController->refreshIfContains(path);
        }
        if (statusBar()) {
            statusBar()->showMessage(tr("File saved successfully"), 2000);
        }
    });

    m_editorStatus = new EditorStatusWidget(this);
    statusBar()->addPermanentWidget(m_editorStatus);
    connect(m_editorManager, &EditorManager::currentChanged, this, [this]() {
        m_editorStatus->setEditor(currentEditor());
    });
    m_editorStatus->setEditor(currentEditor());
}

void MainWindow::setupFileTree()
{
    m_workspaceController = new WorkspaceController(ui->fileTreeWidget, this);

    connect(m_workspaceController, &WorkspaceController::fileActivated,
            this, &MainWindow::openFileInEditor);
    connect(m_workspaceController, &WorkspaceController::fileChangedOnDisk,
            this, &MainWindow::onFileChangedOnDisk);
    connect(m_workspaceController, &WorkspaceController::fileSelected, this, [this](const QString &path) {
        if (statusBar()) {
            statusBar()->showMessage(tr("Selected: %1").arg(QFileInfo(path).fileName()), 2000);
        }
    });
    connect(m_workspaceController->explorer(), &FileExplorer::newFileRequested,
            this, &MainWindow::newFile);
    connect(m_workspaceController->explorer(), &FileExplorer::newFolderRequested,
            this, &MainWindow::newFolder);
    connect(m_workspaceController->explorer(), &FileExplorer::openInTerminalRequested,
            this, [this](const QString &directory) {
                if (m_terminalPanel) {
                    m_terminalPanel->addTerminal(directory);
                }
            });
}

void MainWindow::setupTerminalPanel()
{
    m_terminalPanel = new TerminalPanel(this);
    if (ui->verticalLayout) {
        ui->verticalLayout->addWidget(m_terminalPanel);
    }

    connect(m_terminalPanel, &TerminalPanel::addRequested, this, [this]() {
        m_terminalPanel->addTerminal(editorDirectoryOrWorkspace());
    });
    connect(m_terminalPanel, &TerminalPanel::currentTabChanged, this, [this]() {
        m_terminalPanel->setCurrentWorkingDirectory(editorDirectoryOrWorkspace());
    });
    connect(m_terminalPanel, &TerminalPanel::editorFocusRequested, this, [this]() {
        if (currentEditor()) {
            currentEditor()->setFocus();
        }
    });
}

void MainWindow::installChatWidget()
{
    chatWidget = new ChatWidget(this);
    const QString root = workspaceRoot();
    if (!root.isEmpty()) {
        chatWidget->setProjectDirectory(root);
    }
    replacePlaceholder(ui->rightChatPlaceholder, chatWidget, ui->rightSidebar);
}

void MainWindow::newFile()
{
    const QString fileName = promptText(this,
                                        tr("New File"),
                                        tr("Enter file name:"),
                                        tr("untitled.txt"),
                                        800,
                                        150);
    if (fileName.isEmpty() || !m_workspaceController) {
        return;
    }

    QString fullPath;
    if (m_workspaceController->createEmptyFile(fileName, &fullPath)) {
        openFileInEditor(fullPath);
        QMessageBox::information(this, tr("Success"), tr("File created successfully!"));
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Could not create file!"));
    }
}

void MainWindow::newFolder()
{
    const QString folderName = promptText(this,
                                          tr("New Folder"),
                                          tr("Enter folder name:"),
                                          tr("New Folder"),
                                          400,
                                          150);
    if (folderName.isEmpty() || !m_workspaceController) {
        return;
    }

    QString fullPath;
    if (m_workspaceController->createDirectory(folderName, &fullPath)) {
        QMessageBox::information(this, tr("Success"), tr("Folder created successfully!"));
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Could not create folder!"));
    }
}

void MainWindow::openFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Open File"),
                                                          workspaceRoot(),
                                                          tr("All Files (*.*);;Text Files (*.txt);;C++ Files (*.cpp *.h);;Python Files (*.py)"));

    if (!fileName.isEmpty()) {
        openFileInEditor(fileName);
    }
}

void MainWindow::openFolder()
{
    const QString start = workspaceRoot().isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        : workspaceRoot();
    const QString folderPath = QFileDialog::getExistingDirectory(this, tr("Open Folder"), start);

    if (!folderPath.isEmpty()) {
        setProjectDirectory(folderPath);
    }
}

void MainWindow::onAddFileClicked()
{
    newFile();
}

void MainWindow::onNewFolderClicked()
{
    newFolder();
}

void MainWindow::onCloseExplorerClicked()
{
    isFileTreeVisible = !isFileTreeVisible;

    if (isFileTreeVisible) {
        ui->fileTreeWidget->setVisible(true);
        ui->closeExplorerButton->setText("▼");
    } else {
        ui->fileTreeWidget->setVisible(false);
        ui->closeExplorerButton->setText("▶");
    }
}

void MainWindow::openPreferences()
{
    SettingsDialog dialog(m_keybindings, m_commands, this);
    connect(&dialog, &SettingsDialog::settingsApplied, this, &MainWindow::applyPreferences);
    dialog.exec();
}

void MainWindow::openCommandPalette()
{
    CommandPaletteDialog dialog(m_commands, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QAction *action = m_commands ? m_commands->action(dialog.selectedId()) : nullptr;
    if (action && action->isEnabled()) {
        action->trigger();
    }
}

void MainWindow::openQuickOpen()
{
    if (!m_workspaceController) {
        return;
    }
    const QStringList extra = m_editorManager ? m_editorManager->openFilePaths() : QStringList{};
    QuickOpenDialog dialog(m_workspaceController->fileIndex(), extra, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const QString path = dialog.selectedPath();
    if (path.isEmpty()) {
        return;
    }
    openFileInEditor(path);
    if (dialog.selectedLine() > 0 && m_editorManager) {
        m_editorManager->goToLine(dialog.selectedLine());
    }
}

void MainWindow::openWorkspaceSearch()
{
    if (!m_workspaceController || !m_editorManager) {
        return;
    }
    if (!m_searchDialog) {
        m_searchDialog = new WorkspaceSearchDialog(m_workspaceController, m_editorManager, this);
        connect(m_searchDialog, &WorkspaceSearchDialog::openHitRequested, this,
                [this](const QString &path, int line, int column) {
                    openFileInEditor(path);
                    if (m_editorManager) {
                        m_editorManager->goToLine(line, column);
                    }
                });
        connect(m_searchDialog, &WorkspaceSearchDialog::fileMutated, this,
                [this](const QString &path) {
                    if (m_workspaceController) {
                        m_workspaceController->ignoreNextChange(path);
                        m_workspaceController->refreshIfContains(path);
                    }
                });
    }
    m_searchDialog->show();
    m_searchDialog->raise();
    m_searchDialog->activateWindow();
}

void MainWindow::applyPreferences()
{
    const AppSettings settings;
    if (qApp) {
        ThemeManager::apply(*qApp, settings.themeId());
    }
    if (m_editorManager) {
        m_editorManager->applySettings();
    }
    if (m_workspaceController) {
        m_workspaceController->reloadExplorer();
    }
    if (ui->checkBox) {
        const QSignalBlocker blocker(ui->checkBox);
        ui->checkBox->setChecked(settings.editorLineNumbers());
    }
    restartAutoSave();
}

void MainWindow::restartAutoSave()
{
    if (!m_autoSaveTimer) {
        m_autoSaveTimer = new QTimer(this);
        connect(m_autoSaveTimer, &QTimer::timeout, this, [this]() {
            if (m_editorManager && AppSettings().autoSave()) {
                m_editorManager->saveDirtyFilesQuietly();
            }
        });
    }

    const AppSettings settings;
    if (!settings.autoSave()) {
        m_autoSaveTimer->stop();
        return;
    }
    m_autoSaveTimer->start(settings.autoSaveDelayMs());
}

void MainWindow::onShowLinesToggled(bool checked)
{
    AppSettings().setEditorLineNumbers(checked);
    if (m_editorManager) {
        m_editorManager->applySettings();
    }
}

void MainWindow::openFileInEditor(const QString &filePath)
{
    if (!m_viewerManager) {
        return;
    }

    const ViewerManager::OpenResult result = m_viewerManager->open(filePath);
    if (result == ViewerManager::OpenResult::Opened) {
        ui->centralStack->setCurrentIndex(CodeViewer);
        if (ViewerManager::kindForPath(filePath) != ViewerManager::FileKind::Text) {
            raise();
            activateWindow();
        }
        syncFileWatches();
        saveSession();
        return;
    }
    if (result == ViewerManager::OpenResult::Unsupported) {
        ui->centralStack->setCurrentIndex(UnsupportedViewer);
    }
}

void MainWindow::saveCurrentDocument()
{
    if (m_editorManager) {
        m_editorManager->saveCurrent();
    }
}

void MainWindow::saveCurrentDocumentAs()
{
    if (m_editorManager) {
        m_editorManager->saveCurrentAs();
    }
}

void MainWindow::saveAllDocuments()
{
    if (m_editorManager) {
        m_editorManager->saveAll();
    }
}

void MainWindow::closeCurrentTab()
{
    if (m_editorManager) {
        m_editorManager->closeCurrent();
    }
}

void MainWindow::closeOtherTabs()
{
    if (m_editorManager) {
        m_editorManager->closeOthers();
    }
}

void MainWindow::closeAllTabs()
{
    if (m_editorManager) {
        m_editorManager->closeAll();
    }
}

void MainWindow::updateWindowTitle()
{
    QString title = QStringLiteral("Editerako");

    if (m_editorManager) {
        const QString path = m_editorManager->currentFilePath();
        if (!path.isEmpty()) {
            title += QStringLiteral(" - ") + QFileInfo(path).fileName();
        } else if (m_editorManager->currentEditor()) {
            title += QStringLiteral(" - ") + tr("untitled");
        }

        if (auto *doc = m_editorManager->currentDocument(); doc && doc->isModified()) {
            title += QLatin1Char('*');
        }
    }

    setWindowTitle(title);
}

CodeEditor *MainWindow::currentEditor()
{
    return m_editorManager ? m_editorManager->currentEditor() : nullptr;
}

QString MainWindow::workspaceRoot() const
{
    return m_workspaceController ? m_workspaceController->rootPath() : QString();
}

QString MainWindow::editorDirectoryOrWorkspace() const
{
    if (m_editorManager) {
        const QString path = m_editorManager->currentFilePath();
        if (!path.isEmpty()) {
            return QFileInfo(path).absolutePath();
        }
    }
    return workspaceRoot();
}

void MainWindow::onActionFindReplace()
{
    CodeEditor *ed = currentEditor();
    if (!ed) {
        QMessageBox::information(this, tr("Find"), tr("No text editor is active."));
        return;
    }
    FindReplaceDialog dlg(ed, this);
    dlg.exec();
}

void MainWindow::onActionGoToLine()
{
    CodeEditor *ed = currentEditor();
    if (!ed) {
        QMessageBox::information(this, tr("Go to Line"), tr("No text editor is active."));
        return;
    }
    GoToLineDialog dlg(ed, this);
    dlg.exec();
}

void MainWindow::toggleTerminal()
{
    if (m_terminalPanel) {
        m_terminalPanel->toggle(editorDirectoryOrWorkspace());
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_editorManager && !m_editorManager->promptSaveAllOnQuit()) {
        event->ignore();
        return;
    }

    if (m_lsp) {
        m_lsp->stopAll();
    }

    if (m_terminalPanel) {
        m_terminalPanel->shutdownAll();
    }

    if (chatWidget) {
        chatWidget->saveChatHistory();
    }
    saveSession();
    event->accept();
}

void MainWindow::focusMainWindowAndEditor()
{
    raise();
    activateWindow();
    if (currentEditor()) {
        currentEditor()->setFocus();
    }
}

void MainWindow::promptOpenFolderOrFile()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Welcome to Editerako"));
    msgBox.setText(tr("What would you like to open?"));
    msgBox.setIcon(QMessageBox::Question);

    QPushButton *folderBtn = msgBox.addButton(tr("Open Folder"), QMessageBox::AcceptRole);
    QPushButton *fileBtn = msgBox.addButton(tr("Open File"), QMessageBox::AcceptRole);
    msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);

    msgBox.exec();

    const QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    if (msgBox.clickedButton() == folderBtn) {
        const QString folderPath = QFileDialog::getExistingDirectory(this, tr("Open Folder"), documents);
        setProjectDirectory(folderPath.isEmpty() ? documents : folderPath);
    } else if (msgBox.clickedButton() == fileBtn) {
        const QString fileName = QFileDialog::getOpenFileName(this,
                                                              tr("Open File"),
                                                              documents,
                                                              tr("All Files (*.*)"));
        if (!fileName.isEmpty()) {
            setProjectDirectory(QFileInfo(fileName).absolutePath());
            openFileInEditor(fileName);
        } else {
            setProjectDirectory(documents);
        }
    } else {
        setProjectDirectory(documents);
    }

    focusMainWindowAndEditor();
}

void MainWindow::setProjectDirectory(const QString &path)
{
    if (m_workspaceController) {
        m_workspaceController->setRootPath(path);
    }
}

void MainWindow::syncChatContext()
{
    if (!chatWidget) {
        return;
    }

    CodeEditor *editor = m_editorManager ? m_editorManager->currentEditor() : nullptr;
    if (!editor) {
        chatWidget->setActiveFileContext({}, {});
        return;
    }

    QString path;
    if (EditorDocument *doc = EditorDocument::fromEditor(editor)) {
        path = doc->filePath();
        if (path.isEmpty()) {
            path = doc->displayName();
        }
    }
    chatWidget->setActiveFileContext(path, editor->toPlainText());
}

void MainWindow::saveSession()
{
    SessionState state;
    state.workspace = workspaceRoot();
    if (m_editorManager) {
        state.openFiles = m_editorManager->openFilePaths();
        state.activeFile = m_editorManager->currentFilePath();
    }
    state.geometry = saveGeometry();
    state.windowState = saveState();
    m_session.save(state);
}

bool MainWindow::restoreSession()
{
    const SessionState state = m_session.load();
    if (!state.geometry.isEmpty()) {
        restoreGeometry(state.geometry);
    }
    if (!state.windowState.isEmpty()) {
        restoreState(state.windowState);
    }

    if (!SessionController::workspaceIsRestorable(state)) {
        return false;
    }

    SessionController::RestoreGuard guard(m_session);
    setProjectDirectory(state.workspace);
    if (m_editorManager) {
        m_editorManager->applySettings();
    }

    const QStringList toOpen = SessionController::existingFiles(state.openFiles);
    for (const QString &path : toOpen) {
        openFileInEditor(path);
    }
    if (!toOpen.isEmpty() && m_editorManager) {
        m_editorManager->closeUntitledIfPristine();
    }
    if (m_editorManager && !state.activeFile.isEmpty()) {
        m_editorManager->activateExisting(state.activeFile);
        ui->centralStack->setCurrentIndex(CodeViewer);
    }

    syncFileWatches();
    updateCommandStates();
    return true;
}

void MainWindow::syncFileWatches()
{
    if (!m_workspaceController) {
        return;
    }
    const QStringList openFiles = m_editorManager ? m_editorManager->openFilePaths() : QStringList();
    m_workspaceController->syncWatchedFiles(openFiles);
}

void MainWindow::onFileChangedOnDisk(const QString &path)
{
    if (!m_editorManager) {
        return;
    }

    CodeEditor *editor = m_editorManager->editorForPath(path);
    if (!editor) {
        return;
    }

    switch (diskChangeAction(QFileInfo::exists(path), editor->document()->isModified())) {
    case DiskChangeAction::WarnDeletedDirty:
        QMessageBox::warning(
            this,
            tr("File deleted"),
            tr("The file \"%1\" was deleted on disk. Your unsaved changes are still in the editor.")
                .arg(QFileInfo(path).fileName()));
        syncFileWatches();
        return;
    case DiskChangeAction::CloseTab: {
        const int idx = m_editorManager->tabWidget()->indexOf(editor);
        if (idx >= 0) {
            m_editorManager->closeTab(idx);
        }
        syncFileWatches();
        return;
    }
    case DiskChangeAction::PromptReload: {
        const auto result = QMessageBox::question(
            this,
            tr("File changed"),
            tr("The file \"%1\" has changed on disk. Reload and discard unsaved changes?")
                .arg(QFileInfo(path).fileName()),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (result != QMessageBox::Yes) {
            return;
        }
        break;
    }
    case DiskChangeAction::Reload:
        break;
    }

    if (m_editorManager->reloadFromDisk(path)) {
        if (statusBar()) {
            statusBar()->showMessage(tr("Reloaded %1").arg(QFileInfo(path).fileName()), 2000);
        }
        syncChatContext();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (!mimeData->hasUrls()) {
        return;
    }

    const QStringList paths = localPathsFromMimeData(mimeData);
    for (const QString &filePath : paths) {
        QFileInfo fileInfo(filePath);
        if (fileInfo.isFile()) {
            openFileInEditor(filePath);
        } else if (fileInfo.isDir()) {
            setProjectDirectory(filePath);
        }
    }

    event->acceptProposedAction();
}
