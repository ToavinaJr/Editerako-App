#ifndef EDITERAKO_MAINWINDOW_H
#define EDITERAKO_MAINWINDOW_H

#include "core/SessionController.h"

#include <QList>
#include <QMainWindow>

class ChatWidget;
class CodeEditor;
class CommandRegistry;
class EditorManager;
class EditorStatusWidget;
class KeybindingManager;
class LspServerManager;
class LspSession;
class DebugSession;
class BottomPanel;
class TerminalPanel;
class ViewerManager;
class WorkspaceController;
class WorkspaceSearchDialog;
class GitCliProvider;
class TaskManager;
class PluginManager;
class QMenu;

class QCloseEvent;
class QDragEnterEvent;
class QDropEvent;
class QTimer;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

    enum ViewerIndex {
        CodeViewer = 0,
        UnsupportedViewer
    };

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void newFile();
    void newFolder();
    void openFile();
    void openFolder();

    void saveCurrentDocument();
    void saveCurrentDocumentAs();
    void saveAllDocuments();
    void closeCurrentTab();
    void closeOtherTabs();
    void closeAllTabs();
    void splitEditorRight();
    void splitEditorDown();
    void moveEditor();
    void closeEditorGroup();

    void onAddFileClicked();
    void onNewFolderClicked();
    void onCloseExplorerClicked();

    void onShowLinesToggled(bool checked);

    void onActionFindReplace();
    void onActionGoToLine();

    void toggleTerminal();
    void toggleProblems();
    void toggleSourceControl();
    void toggleTasks();
    void toggleOutput();
    void toggleDebug();
    void runBuildTask();
    void compareWithDisk();
    void openMarkdownPreview();
    void openPreferences();
    void openCommandPalette();
    void openQuickOpen();
    void openWorkspaceSearch();

private:
    Ui::MainWindow *ui;
    CommandRegistry *m_commands = nullptr;
    KeybindingManager *m_keybindings = nullptr;
    EditorManager *m_editorManager = nullptr;
    EditorStatusWidget *m_editorStatus = nullptr;
    ViewerManager *m_viewerManager = nullptr;
    LspServerManager *m_lsp = nullptr;
    LspSession *m_lspSession = nullptr;
    DebugSession *m_debugSession = nullptr;
    BottomPanel *m_bottomPanel = nullptr;
    WorkspaceController *m_workspaceController = nullptr;
    TerminalPanel *m_terminalPanel = nullptr;
    WorkspaceSearchDialog *m_searchDialog = nullptr;
    GitCliProvider *m_scm = nullptr;
    TaskManager *m_tasks = nullptr;
    PluginManager *m_pluginManager = nullptr;
    QMenu *m_pluginMenu = nullptr;
    ChatWidget *chatWidget = nullptr;
    SessionController m_session;
    QTimer *m_autoSaveTimer = nullptr;
    QTimer *m_backupTimer = nullptr;
    bool isFileTreeVisible = true;

    CodeEditor *currentEditor();
    QString editorDirectoryOrWorkspace() const;
    QString workspaceRoot() const;

    void connectActions();
    void connectWorkspaceCollaborators();
    void applyPreferences();
    void restartAutoSave();
    void updateCommandStates();
    void updateWindowTitle();
    void setupFileTree();
    void setupCodeEditor();
    void setupBottomPanel();
    void installChatWidget();
    void setupPlugins();
    void focusMainWindowAndEditor();
    void openFileInEditor(const QString &filePath);
    void promptOpenFolderOrFile();
    void setProjectDirectory(const QString &path);
    void syncChatContext();
    void saveSession();
    bool restoreSession();
    void scheduleRecoveryBackup();
    void writeRecoveryBackup();
    bool restoreRecoveryBackups();
    [[nodiscard]] QList<CodeEditor *> modifiedSecretEditors() const;
    void syncFileWatches();
    void onFileChangedOnDisk(const QString &path);
};

#endif // EDITERAKO_MAINWINDOW_H
