#include "app/MainWindow.h"
#include "ui_MainWindow.h"

#include "ai/ChatWidget.h"
#include "app/DebugSession.h"
#include "app/LspSession.h"
#include "core/AppSettings.h"
#include "editor/CodeEditor.h"
#include "editor/EditorManager.h"
#include "editor/EditorStatusWidget.h"
#include "editor/EditorDiagnostic.h"
#include "editor/ProblemModel.h"
#include "plugins/IFileViewerProvider.h"
#include "plugins/PluginManager.h"
#include "project/FileExplorer.h"
#include "project/WorkspaceController.h"
#include "scm/GitCliProvider.h"
#include "scm/GitParsers.h"
#include "tasks/TaskDefinition.h"
#include "tasks/TaskManager.h"
#include "terminal/TerminalPanel.h"
#include "ui/BottomPanel.h"
#include "ui/ProblemsPanel.h"
#include "ui/SourceControlPanel.h"
#include "ui/TasksPanel.h"
#include "ui/UiHelpers.h"
#include "viewers/ViewerManager.h"

#include <QFileInfo>
#include <QMenu>
#include <QMenuBar>
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
                if (m_debugSession) {
                    m_debugSession->setWorkspaceRoot(path);
                }
                if (m_pluginManager) {
                    m_pluginManager->setWorkspaceRoot(path);
                }
                if (m_bottomPanel) {
                    m_bottomPanel->problemsPanel()->setWorkspaceRoot(path);
                    m_bottomPanel->sourceControlPanel()->setWorkspace(path);
                    m_bottomPanel->tasksPanel()->setWorkspace(path);
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

    ui->centralStack->insertWidget(CodeViewer, m_editorManager->containerWidget());
    ui->centralStack->setCurrentIndex(CodeViewer);

    m_viewerManager = new ViewerManager(m_editorManager, this);
    connect(m_editorManager, &EditorManager::viewerDuplicateRequested,
            this, [this](const QString &path) {
                m_viewerManager->openNew(path);
            });

    connect(m_editorManager, &EditorManager::currentChanged, this, &MainWindow::updateWindowTitle);
    connect(m_editorManager, &EditorManager::currentChanged, this, &MainWindow::syncChatContext);
    connect(m_editorManager, &EditorManager::currentChanged, this, &MainWindow::updateCommandStates);
    connect(m_editorManager, &EditorManager::currentChanged, this, &MainWindow::saveSession);
    connect(m_editorManager, &EditorManager::currentChanged, this, &MainWindow::syncFileWatches);
    connect(m_editorManager, &EditorManager::modificationChanged, this, &MainWindow::updateWindowTitle);
    connect(m_editorManager, &EditorManager::modificationChanged, this, &MainWindow::scheduleRecoveryBackup);
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
        scheduleRecoveryBackup();
    });

    m_editorStatus = new EditorStatusWidget(this);
    statusBar()->addPermanentWidget(m_editorStatus);
    connect(m_editorManager, &EditorManager::currentChanged, this, [this]() {
        m_editorStatus->setEditor(currentEditor());
        if (m_tasks) {
            m_tasks->setActiveFile(m_editorManager->currentFilePath());
        }
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
        if (AppSettings().editorPreviewTabs()) {
            openFileInEditor(path, true);
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
    m_tasks = new TaskManager(this);
    m_bottomPanel = new BottomPanel(m_scm, m_tasks, m_debugSession, this);
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
    if (m_debugSession) {
        connect(m_debugSession, &DebugSession::panelRevealRequested, m_bottomPanel,
                &BottomPanel::showDebug);
    }
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
    if (m_tasks) {
        connect(m_tasks, &TaskManager::problemsMatched, this,
                [this](const QVector<TaskProblem> &problems) {
                    QVector<ProblemItem> items;
                    items.reserve(problems.size());
                    for (const TaskProblem &problem : problems) {
                        ProblemItem item;
                        item.path = problem.path;
                        item.line = problem.line;
                        item.column = problem.column;
                        item.message = problem.message;
                        item.source = QStringLiteral("task");
                        switch (problem.severity) {
                        case TaskProblem::Severity::Warning:
                            item.severity = EditorDiagnostic::Severity::Warning;
                            break;
                        case TaskProblem::Severity::Information:
                            item.severity = EditorDiagnostic::Severity::Information;
                            break;
                        case TaskProblem::Severity::Error:
                            item.severity = EditorDiagnostic::Severity::Error;
                            break;
                        }
                        items.append(item);
                    }
                    m_bottomPanel->problemsPanel()->model()->setSourceProblems(
                        QStringLiteral("task"), items);
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

void MainWindow::setupPlugins()
{
    m_pluginManager = new PluginManager(this);
    m_pluginManager->setCommandRegistry(m_commands);
    m_pluginManager->setDialogParent(this);
    m_pluginManager->setDisabledIds(AppSettings().disabledPlugins());
    if (m_viewerManager) {
        m_pluginManager->setViewerHandlers(
            [this](IFileViewerProvider *provider) { m_viewerManager->addProvider(provider); },
            [this](IFileViewerProvider *provider) { m_viewerManager->removeProvider(provider); });
    }
    if (m_bottomPanel) {
        m_pluginManager->setPanelHandlers(
            [this](const QString &id, const QString &title, QWidget *widget) {
                m_bottomPanel->addPluginTab(id, title, widget);
            },
            [this](const QString &id) { m_bottomPanel->removePluginTab(id); });
    }
    connect(m_pluginManager, &PluginManager::statusMessage, this,
            [this](const QString &message, int timeoutMs) {
                if (statusBar()) {
                    statusBar()->showMessage(message, timeoutMs);
                }
            });

    m_pluginMenu = menuBar()->addMenu(tr("Plugins"));
    connect(m_pluginManager, &PluginManager::pluginsChanged, this, [this]() {
        if (m_pluginManager && m_pluginMenu) {
            m_pluginManager->fillMenu(m_pluginMenu);
        }
    });
    m_pluginManager->reload();
}
