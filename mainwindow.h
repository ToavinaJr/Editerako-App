#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QTreeWidgetItem>
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

private:
    QPdfDocument *pdfDoc = nullptr;
    QPdfView *pdfView = nullptr;
    QLabel *imageLabel = nullptr;
    QScrollArea *imageScroll = nullptr;

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
    // File menu operations
    void newFile();
    void newFolder();
    void openFile();
    void openFolder();

    // Explorer buttons
    void onAddFileClicked();
    void onNewFolderClicked();
    void onCloseExplorerClicked();

    // File tree interaction
    void onFileTreeItemClicked(QTreeWidgetItem *item, int column);
    void onFileTreeItemDoubleClicked(QTreeWidgetItem *item, int column);

    // Line numbers toggle
    void onShowLinesToggled(bool checked);

    // Find section
    void onActionFindReplace();
    void onActionGoToLine();

    // Ouvrir terminal
    void toggleTerminal();
    void onTerminalClosed();

    // Nouvelles méthodes pour les terminaux multiples
    void addNewTerminal();
    void closeTerminalTab(int index);
    void onTerminalTabChanged(int index);

private:
    Ui::MainWindow *ui;
    QTabWidget *editorTabs;
    QString currentFileName;
    QString currentWorkingDirectory;
    // Terminal *terminal; // Ancien terminal unique, remplacé par terminalList
    QShortcut *terminalShortcut;
    ChatWidget *chatWidget = nullptr;
    bool isTerminalVisible;
    bool isModified;

    // Nouveaux membres pour les terminaux multiples
    QTabWidget *terminalTabs;
    QList<Terminal*> terminalList;
    QPushButton *addTerminalButton;

    // État de l'explorateur
    bool isFileTreeVisible;

    CodeEditor *currentEditor();
private slots:
    void onEditorTabChanged(int index);
    void closeTab(int index);

private:
    bool saveEditor(CodeEditor *editor);
    void updateTabModifiedState(CodeEditor *editor);
    void updateTabLabel(CodeEditor *editor);

    void connectActions();
    void updateWindowTitle();
    void setupFileTree();
    void setupCodeEditor();
    void setupTerminalTabs();
    void loadDirectoryToTree(const QString &path);
    void addFileToTree(const QString &fileName, QTreeWidgetItem *parent = nullptr);
    void addFolderToTree(const QString &folderName, QTreeWidgetItem *parent = nullptr);
    bool askToSaveChanges();
    QString getFileExtension(const QString &fileName);
    QString getFileIcon(const QString &fileName);
    void openFileInEditor(const QString &filePath);
    void saveCurrentFile();
    void promptOpenFolderOrFile();
    void setProjectDirectory(const QString &path);
};

#endif // MAINWINDOW_H
