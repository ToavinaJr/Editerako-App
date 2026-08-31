#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QInputDialog>
#include <QDir>
#include <QFileInfo>
#include <QStatusBar>
#include <QCheckBox>
#include <QStackedWidget>
#include <QPdfDocument>
#include <QtPdfWidgets/QPdfView>
#include <QScrollArea>
#include <QLabel>
#include <QShortcut>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QList>
#include <QUrl>
#include <QMenu>
#include "terminal.h"
#include "codeeditor.h"
#include <QTabWidget>
#include <QTabBar>

class ChatWidget;
class EditorManager;
class FileExplorer;
class Workspace;

QT_BEGIN_NAMESPACE
class QAction;
class QMenuBar;
class QStatusBar;
QT_END_NAMESPACE

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    enum ViewerIndex {
        CodeViewer = 0,
        PdfViewer,
        ImageViewer,
        UnsupportedViewer
    };

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

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

    void onAddFileClicked();
    void onNewFolderClicked();
    void onCloseExplorerClicked();

    void onShowLinesToggled(bool checked);

    void onActionFindReplace();
    void onActionGoToLine();

    void toggleTerminal();
    void onTerminalClosed();

    void addNewTerminal();
    void closeTerminalTab(int index);
    void onTerminalTabChanged(int index);

private:
    Ui::MainWindow *ui;
    EditorManager *m_editorManager = nullptr;
    Workspace *m_workspace = nullptr;
    FileExplorer *m_fileExplorer = nullptr;
    QString currentWorkingDirectory;
    QShortcut *terminalShortcut = nullptr;
    ChatWidget *chatWidget = nullptr;
    bool isTerminalVisible = false;
    QTabWidget *terminalTabs = nullptr;
    QList<Terminal*> terminalList;
    QPushButton *addTerminalButton = nullptr;
    bool isFileTreeVisible = true;

    QPdfDocument *pdfDoc = nullptr;
    QPdfView *pdfView = nullptr;
    QLabel *imageLabel = nullptr;
    QScrollArea *imageScroll = nullptr;

    CodeEditor *currentEditor();
    QString editorDirectoryOrWorkspace() const;

    void connectActions();
    void updateWindowTitle();
    void setupFileTree();
    void setupCodeEditor();
    void setupTerminalTabs();
    void openFileInEditor(const QString &filePath);
    void promptOpenFolderOrFile();
    void setProjectDirectory(const QString &path);
};

#endif // MAINWINDOW_H
