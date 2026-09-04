#include "app/MainWindow.h"
#include "ui_MainWindow.h"

#include "ai/ChatWidget.h"
#include "app/DebugSession.h"
#include "app/LspSession.h"
#include "core/AppSettings.h"
#include "core/BackupService.h"
#include "core/RecoveryService.h"
#include "core/ThemeManager.h"
#include "editor/CodeEditor.h"
#include "editor/EditorDocument.h"
#include "editor/EditorManager.h"
#include "editor/EditorStatusWidget.h"
#include "lsp/LspServerManager.h"
#include "plugins/IFileViewerProvider.h"
#include "plugins/PluginManager.h"
#include "project/WorkspaceController.h"
#include "terminal/TerminalPanel.h"
#include "tasks/TaskManager.h"
#include "ui/BottomPanel.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QFileInfo>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QStringList>
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

    m_debugSession = new DebugSession(m_editorManager, this);
    connect(m_debugSession, &DebugSession::statusMessage, this,
            [this](const QString &message, int timeoutMs) {
                if (statusBar()) {
                    statusBar()->showMessage(message, timeoutMs);
                }
            });
    connect(m_debugSession, &DebugSession::debugStatusChanged, this, [this](const QString &text) {
        if (m_editorStatus) {
            m_editorStatus->setDebugStatus(text);
        }
    });
    connect(m_debugSession, &DebugSession::stateChanged, this, [this](DebugSession::State) {
        updateCommandStates();
    });

    connectActions();
    restartAutoSave();
    updateWindowTitle();

    if (currentEditor()) {
        currentEditor()->setFocus();
    }

    setupBottomPanel();
    installChatWidget();
    setupPlugins();
    connectWorkspaceCollaborators();

    setAcceptDrops(true);

    if (!restoreSession()) {
        promptOpenFolderOrFile();
    }
    syncChatContext();
}

MainWindow::~MainWindow()
{
    if (m_pluginManager) {
        m_pluginManager->unloadAll();
    }
    if (m_lsp) {
        m_lsp->stopAll();
    }
    if (m_debugSession) {
        m_debugSession->stop();
    }
    if (m_terminalPanel) {
        m_terminalPanel->shutdownAll();
    }
    delete ui;
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
    if (chatWidget) {
        chatWidget->reloadFromSettings();
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
        m_editorManager->setLineNumbersVisible(checked);
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

void MainWindow::splitEditorRight()
{
    if (m_editorManager) {
        m_editorManager->splitRight();
    }
}

void MainWindow::splitEditorDown()
{
    if (m_editorManager) {
        m_editorManager->splitDown();
    }
}

void MainWindow::moveEditor()
{
    if (m_editorManager) {
        m_editorManager->moveEditor();
    }
}

void MainWindow::closeEditorGroup()
{
    if (m_editorManager) {
        m_editorManager->closeActiveGroup();
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

void MainWindow::toggleTerminal()
{
    if (m_terminalPanel) {
        m_terminalPanel->toggle(editorDirectoryOrWorkspace());
    }
}

void MainWindow::toggleProblems()
{
    if (m_bottomPanel) {
        m_bottomPanel->toggleProblems();
    }
}

void MainWindow::toggleSourceControl()
{
    if (m_bottomPanel) {
        m_bottomPanel->toggleSourceControl();
    }
}

void MainWindow::toggleTasks()
{
    if (m_bottomPanel) {
        m_bottomPanel->toggleTasks();
    }
}

void MainWindow::toggleOutput()
{
    if (m_bottomPanel) {
        m_bottomPanel->toggleOutput();
    }
}

void MainWindow::toggleDebug()
{
    if (m_bottomPanel) {
        m_bottomPanel->toggleDebug();
    }
}

void MainWindow::runBuildTask()
{
    if (m_tasks) {
        m_tasks->runBuild();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_editorManager) {
        if (AppSettings().hotExit()) {
            writeRecoveryBackup();
            const QList<CodeEditor *> secrets = modifiedSecretEditors();
            if (!m_editorManager->promptSaveEditors(secrets, true)) {
                event->ignore();
                return;
            }
            writeRecoveryBackup();
        } else if (!m_editorManager->promptSaveAllOnQuit()) {
            event->ignore();
            return;
        } else {
            RecoveryService().discard();
        }
    }

    if (m_lsp) {
        m_lsp->stopAll();
    }
    if (m_debugSession) {
        m_debugSession->stop();
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

    SessionController::RestoreGuard guard(m_session);
    bool restoredWorkspace = false;
    if (SessionController::workspaceIsRestorable(state)) {
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
        restoredWorkspace = true;
    }

    const bool recovered = restoreRecoveryBackups();
    if (m_editorManager) {
        m_editorManager->closeUntitledIfPristine();
    }

    syncFileWatches();
    updateCommandStates();
    return restoredWorkspace || recovered;
}

void MainWindow::scheduleRecoveryBackup()
{
    if (m_session.isRestoring()) {
        return;
    }
    if (!m_backupTimer) {
        m_backupTimer = new QTimer(this);
        m_backupTimer->setSingleShot(true);
        connect(m_backupTimer, &QTimer::timeout, this, &MainWindow::writeRecoveryBackup);
    }
    m_backupTimer->start(1000);
}

void MainWindow::writeRecoveryBackup()
{
    if (!m_editorManager || m_session.isRestoring()) {
        return;
    }

    BackupSnapshot snapshot;
    snapshot.workspace = workspaceRoot();
    for (const BackupBuffer &buffer : m_editorManager->dirtyBuffers()) {
        if (BackupService::isSecretPath(buffer.originalPath)
            || BackupService::exceedsSizeLimit(buffer.lfText)) {
            continue;
        }
        snapshot.entries.append(buffer);
    }
    RecoveryService().save(snapshot);
}

bool MainWindow::restoreRecoveryBackups()
{
    RecoveryService recovery;
    if (!recovery.canRecover() || !m_editorManager) {
        return false;
    }

    const BackupSnapshot snapshot = recovery.load();
    int restored = 0;
    for (const BackupBuffer &buffer : snapshot.entries) {
        if (m_editorManager->restoreBuffer(buffer)) {
            ++restored;
        }
    }
    if (restored > 0 && statusBar()) {
        statusBar()->showMessage(tr("Recovered %1 unsaved document(s)").arg(restored), 8000);
    }
    return restored > 0;
}

QList<CodeEditor *> MainWindow::modifiedSecretEditors() const
{
    QList<CodeEditor *> secrets;
    if (!m_editorManager) {
        return secrets;
    }
    for (CodeEditor *editor : m_editorManager->modifiedEditors()) {
        auto *doc = EditorDocument::fromEditor(editor);
        if (doc && BackupService::isSecretPath(doc->filePath())) {
            secrets.append(editor);
        }
    }
    return secrets;
}

void MainWindow::syncFileWatches()
{
    if (!m_workspaceController) {
        return;
    }
    const QStringList openFiles = m_editorManager ? m_editorManager->openFilePaths() : QStringList();
    m_workspaceController->syncWatchedFiles(openFiles);
}
