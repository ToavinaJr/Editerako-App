#include "app/MainWindow.h"
#include "ui_MainWindow.h"
#include "ai/ChatWidget.h"
#include "editor/CodeEditor.h"
#include "editor/FindReplaceDialog.h"
#include "editor/GoToLineDialog.h"
#include "core/CommandRegistry.h"
#include "core/DropPaths.h"
#include "core/SessionStore.h"
#include "editor/EditorDocument.h"
#include "editor/EditorManager.h"
#include "project/FileExplorer.h"
#include "project/FileWatcher.h"
#include "project/Workspace.h"
#include "terminal/TerminalPanel.h"
#include "ui/UiHelpers.h"
#include "viewers/ViewerManager.h"

#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDir>
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
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_editorManager(nullptr)
    , currentWorkingDirectory()
    , isFileTreeVisible(true)
{

    ui->setupUi(this);
    ui->centralStack->setCurrentIndex(CodeViewer);

    setupCodeEditor();
    connectActions();
    setupFileTree();
    updateWindowTitle();

    if (currentEditor()) {
        currentEditor()->setFocus();
    }

    setupTerminalPanel();
    installChatWidget();

    setAcceptDrops(true);

    m_fileWatcher = new FileWatcher(this);
    connect(m_fileWatcher, &FileWatcher::rootContentsChanged, this, [this]() {
        if (m_fileExplorer) {
            m_fileExplorer->reload();
        }
    });
    connect(m_fileWatcher, &FileWatcher::fileChangedOnDisk, this, &MainWindow::onFileChangedOnDisk);

    if (!restoreSession()) {
        promptOpenFolderOrFile();
    }
    syncChatContext();
}

MainWindow::~MainWindow()
{
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

    ui->actionFile->setShortcut(QKeySequence::New);
    ui->actionOpen_File->setShortcut(QKeySequence::Open);
    ui->actionSave->setShortcut(QKeySequence::Save);
    ui->actionSave_As->setShortcut(QKeySequence::SaveAs);
    ui->actionClose->setShortcut(QKeySequence(QStringLiteral("Ctrl+W")));
    ui->actionFindReplace->setShortcut(QKeySequence::Find);
    ui->actionGoToLine->setShortcut(QKeySequence(QStringLiteral("Ctrl+G")));

    QAction *toggleTerminal = m_commands->create(
        QStringLiteral("view.terminal"),
        tr("Toggle Terminal"),
        QKeySequence(QStringLiteral("Ctrl+J")));
    connect(toggleTerminal, &QAction::triggered, this, &MainWindow::toggleTerminal);

    QMenu *viewMenu = menuBar()->addMenu(tr("View"));
    viewMenu->addAction(toggleTerminal);

    connect(ui->addFileButton, &QPushButton::clicked, this, &MainWindow::onAddFileClicked);
    connect(ui->newFolderButton, &QPushButton::clicked, this, &MainWindow::onNewFolderClicked);
    connect(ui->closeExplorerButton, &QPushButton::clicked, this, &MainWindow::onCloseExplorerClicked);
    connect(ui->checkBox, &QCheckBox::toggled, this, &MainWindow::onShowLinesToggled);

    updateCommandStates();
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
        if (m_fileWatcher) {
            m_fileWatcher->ignoreNextChange(path);
        }
    });
    connect(m_editorManager, &EditorManager::fileSaved, this, [this](const QString &path) {
        if (m_workspace && m_fileExplorer && m_workspace->containsPath(path)) {
            m_fileExplorer->reload();
        }
        if (statusBar()) {
            statusBar()->showMessage(tr("File saved successfully"), 2000);
        }
    });
}


void MainWindow::setupFileTree()
{
    m_workspace = new Workspace(this);
    m_fileExplorer = new FileExplorer(ui->fileTreeWidget, m_workspace, this);

    connect(m_fileExplorer, &FileExplorer::fileActivated, this, &MainWindow::openFileInEditor);
    connect(m_fileExplorer, &FileExplorer::directoryPopulated, this, [this](const QString &path) {
        if (m_fileWatcher) {
            m_fileWatcher->watchDirectory(path);
        }
    });
    connect(m_fileExplorer, &FileExplorer::fileSelected, this, [this](const QString &path) {
        if (statusBar()) {
            statusBar()->showMessage(tr("Selected: %1").arg(QFileInfo(path).fileName()), 2000);
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
    if (!currentWorkingDirectory.isEmpty()) {
        chatWidget->setProjectDirectory(currentWorkingDirectory);
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
    if (fileName.isEmpty()) {
        return;
    }

    QString fullPath;
    if (Workspace::createEmptyFile(targetDirectory(), fileName, &fullPath)) {
        if (m_fileExplorer) {
            m_fileExplorer->reload();
            m_fileExplorer->revealPath(fullPath);
        }
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
    if (folderName.isEmpty()) {
        return;
    }

    QString fullPath;
    if (Workspace::createDirectory(targetDirectory(), folderName, &fullPath)) {
        if (m_fileExplorer) {
            m_fileExplorer->reload();
            m_fileExplorer->revealPath(fullPath);
        }
        QMessageBox::information(this, tr("Success"), tr("Folder created successfully!"));
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Could not create folder!"));
    }
}


void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open File"),
                                                    currentWorkingDirectory,
                                                    tr("All Files (*.*);;Text Files (*.txt);;C++ Files (*.cpp *.h);;Python Files (*.py)"));

    if (!fileName.isEmpty()) {
        openFileInEditor(fileName);
    }
}

void MainWindow::openFolder()
{
    QString folderPath = QFileDialog::getExistingDirectory(this,
                                                           tr("Open Folder"),
                                                           currentWorkingDirectory.isEmpty()
                                                               ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                                                               : currentWorkingDirectory);

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

void MainWindow::onShowLinesToggled(bool checked)
{
    if (currentEditor()) {
        currentEditor()->setLineNumbersVisible(checked);
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

QString MainWindow::editorDirectoryOrWorkspace() const
{
    if (m_editorManager) {
        const QString path = m_editorManager->currentFilePath();
        if (!path.isEmpty()) {
            return QFileInfo(path).absolutePath();
        }
    }
    return currentWorkingDirectory;
}

QString MainWindow::targetDirectory() const
{
    return m_fileExplorer ? m_fileExplorer->selectedDirectory()
                          : currentWorkingDirectory;
}

void MainWindow::onActionFindReplace() {
    CodeEditor *ed = currentEditor();
    if (!ed) {
        QMessageBox::information(this, tr("Find"), tr("No text editor is active."));
        return;
    }
    FindReplaceDialog dlg(ed, this);
    dlg.exec();
}

void MainWindow::onActionGoToLine() {
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
        QString folderPath = QFileDialog::getExistingDirectory(this, tr("Open Folder"), documents);
        setProjectDirectory(folderPath.isEmpty() ? documents : folderPath);
    } else if (msgBox.clickedButton() == fileBtn) {
        QString fileName = QFileDialog::getOpenFileName(this,
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
    if (m_workspace) {
        m_workspace->setRootPath(path);
        currentWorkingDirectory = m_workspace->rootPath();
    } else {
        currentWorkingDirectory = path;
    }
    if (m_editorManager) {
        m_editorManager->setWorkingDirectory(currentWorkingDirectory);
    }
    if (m_fileExplorer) {
        m_fileExplorer->reload();
    }

    if (m_terminalPanel) {
        m_terminalPanel->setWorkingDirectory(currentWorkingDirectory);
    }

    if (chatWidget) {
        chatWidget->setProjectDirectory(currentWorkingDirectory);
    }

    if (m_fileWatcher) {
        m_fileWatcher->setRootPath(currentWorkingDirectory);
    }
    saveSession();

    updateWindowTitle();
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
    if (m_restoringSession) {
        return;
    }

    SessionState state;
    state.workspace = currentWorkingDirectory;
    if (m_editorManager) {
        state.openFiles = m_editorManager->openFilePaths();
        state.activeFile = m_editorManager->currentFilePath();
    }
    state.geometry = saveGeometry();
    state.windowState = saveState();
    SessionStore().save(state);
}

bool MainWindow::restoreSession()
{
    const SessionState state = SessionStore().load();
    if (!state.geometry.isEmpty()) {
        restoreGeometry(state.geometry);
    }
    if (!state.windowState.isEmpty()) {
        restoreState(state.windowState);
    }

    if (state.workspace.isEmpty() || !QDir(state.workspace).exists()) {
        return false;
    }

    m_restoringSession = true;
    setProjectDirectory(state.workspace);

    int opened = 0;
    for (const QString &path : state.openFiles) {
        if (QFileInfo::exists(path)) {
            openFileInEditor(path);
            ++opened;
        }
    }
    if (opened > 0 && m_editorManager) {
        m_editorManager->closeUntitledIfPristine();
    }
    if (m_editorManager && !state.activeFile.isEmpty()) {
        m_editorManager->activateExisting(state.activeFile);
        ui->centralStack->setCurrentIndex(CodeViewer);
    }

    m_restoringSession = false;
    syncFileWatches();
    updateCommandStates();
    return true;
}

void MainWindow::syncFileWatches()
{
    if (!m_fileWatcher) {
        return;
    }
    m_fileWatcher->setRootPath(currentWorkingDirectory);
    if (m_editorManager) {
        m_fileWatcher->setFilePaths(m_editorManager->openFilePaths());
    }
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

    if (!QFileInfo::exists(path)) {
        if (editor->document()->isModified()) {
            QMessageBox::warning(
                this,
                tr("File deleted"),
                tr("The file \"%1\" was deleted on disk. Your unsaved changes are still in the editor.")
                    .arg(QFileInfo(path).fileName()));
        } else {
            const int idx = m_editorManager->tabWidget()->indexOf(editor);
            if (idx >= 0) {
                m_editorManager->closeTab(idx);
            }
        }
        syncFileWatches();
        return;
    }

    if (editor->document()->isModified()) {
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
