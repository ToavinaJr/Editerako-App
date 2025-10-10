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
#include "codeeditor.h"

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

private:
    Ui::MainWindow *ui;
    CodeEditor *codeEditor;
    QString currentFileName;
    QString currentWorkingDirectory;
    bool isModified;

    void connectActions();
    void updateWindowTitle();
    void setupFileTree();
    void setupCodeEditor();
    void loadDirectoryToTree(const QString &path);
    void addFileToTree(const QString &fileName, QTreeWidgetItem *parent = nullptr);
    void addFolderToTree(const QString &folderName, QTreeWidgetItem *parent = nullptr);
    bool askToSaveChanges();
    QString getFileExtension(const QString &fileName);
    QString getFileIcon(const QString &fileName);
    void openFileInEditor(const QString &filePath);
    void saveCurrentFile();
};

#endif // MAINWINDOW_H
