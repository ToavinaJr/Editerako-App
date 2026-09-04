#include "app/MainWindow.h"
#include "ui_MainWindow.h"

#include "ai/ChatWidget.h"
#include "app/LspSession.h"
#include "core/AppSettings.h"
#include "editor/CodeEditor.h"
#include "editor/EditorManager.h"
#include "editor/EditorStatusWidget.h"
#include "editor/ProblemModel.h"
#include "project/FileExplorer.h"
#include "project/WorkspaceController.h"
#include "scm/GitCliProvider.h"
#include "scm/GitParsers.h"
#include "terminal/TerminalPanel.h"
#include "ui/BottomPanel.h"
#include "ui/ProblemsPanel.h"
#include "ui/SourceControlPanel.h"
#include "ui/UiHelpers.h"
#include "viewers/ViewerManager.h"

#include <QFileInfo>
#include <QStatusBar>
#include <QTabWidget>

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
                if (m_bottomPanel) {
                    m_bottomPanel->problemsPanel()->setWorkspaceRoot(path);
                    m_bottomPanel->sourceControlPanel()->setWorkspace(path);
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
        if (m_scm) {
            m_scm->refresh();
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
                if (m_bottomPanel) {
                    m_bottomPanel->showTerminal();
                }
            });
}

void MainWindow::setupBottomPanel()
{
    m_scm = new GitCliProvider(this);
    m_bottomPanel = new BottomPanel(m_scm, this);
    m_terminalPanel = m_bottomPanel->terminalPanel();
    if (ui->verticalLayout) {
        ui->verticalLayout->addWidget(m_bottomPanel);
    }

    connect(m_terminalPanel, &TerminalPanel::addRequested, this, [this]() {
        m_terminalPanel->addTerminal(editorDirectoryOrWorkspace());
    });
    connect(m_terminalPanel, &TerminalPanel::currentTabChanged, this, [this]() {
        m_terminalPanel->setCurrentWorkingDirectory(editorDirectoryOrWorkspace());
    });
    connect(m_bottomPanel, &BottomPanel::editorFocusRequested, this, [this]() {
        if (currentEditor()) {
            currentEditor()->setFocus();
        }
    });
    connect(m_bottomPanel->problemsPanel(), &ProblemsPanel::problemActivated, this,
            [this](const QString &path, int line, int column) {
                if (m_editorManager) {
                    m_editorManager->revealLocation(path, line, column);
                }
            });
    if (m_lspSession) {
        connect(m_lspSession, &LspSession::problemsChanged, this,
                [this](const QString &path, const QVector<ProblemItem> &items) {
                    m_bottomPanel->problemsPanel()->model()->setFileProblems(path, items);
                });
    }
    connect(m_bottomPanel->problemsPanel()->model(), &ProblemModel::changed, this, [this]() {
        auto *model = m_bottomPanel->problemsPanel()->model();
        if (m_editorStatus) {
            m_editorStatus->setProblemCounts(model->errorCount(), model->warningCount());
        }
    });
    if (m_editorStatus) {
        connect(m_editorStatus, &EditorStatusWidget::problemsActivated, this,
                &MainWindow::toggleProblems);
    }
    if (m_scm) {
        connect(m_scm, &GitCliProvider::statusChanged, this, [this](const ScmStatus &status) {
            if (m_workspaceController) {
                m_workspaceController->explorer()->setPathBadges(GitParsers::explorerBadges(status));
            }
            if (!m_editorStatus) {
                return;
            }
            const QString branch = GitParsers::branchName(status);
            if (branch.isEmpty()) {
                m_editorStatus->setGitBranch(status.isRepository ? tr("Git: detached") : QString());
            } else {
                m_editorStatus->setGitBranch(tr("Git: %1").arg(branch));
            }
        });
    }
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
