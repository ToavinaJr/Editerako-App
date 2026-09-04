#include "app/MainWindow.h"
#include "ui_MainWindow.h"

#include "core/DiskChangePolicy.h"
#include "core/DropPaths.h"
#include "editor/CodeEditor.h"
#include "editor/EditorIo.h"
#include "editor/EditorManager.h"
#include "project/WorkspaceController.h"
#include "scm/GitCliProvider.h"
#include "scm/TextDiff.h"
#include "ui/BottomPanel.h"
#include "ui/UiHelpers.h"
#include "viewers/FileKind.h"
#include "viewers/ViewerManager.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabWidget>

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

void MainWindow::compareWithDisk()
{
    CodeEditor *editor = currentEditor();
    if (!editor || !m_editorManager || !m_bottomPanel) {
        return;
    }
    const QString path = m_editorManager->currentFilePath();
    if (path.isEmpty()) {
        QMessageBox::information(this, tr("Compare with Disk"),
                                 tr("Save the file before comparing it with disk."));
        return;
    }
    const TextLoadResult loaded = readTextFile(path);
    if (!loaded.ok) {
        QMessageBox::warning(this, tr("Compare with Disk"), loaded.error);
        return;
    }
    m_bottomPanel->showDiff(tr("%1 (disk ↔ editor)").arg(QFileInfo(path).fileName()),
                            TextDiff::unified(loaded.text, editor->toPlainText(),
                                              QStringLiteral("disk"), QStringLiteral("editor")));
}

void MainWindow::openMarkdownPreview()
{
    if (!m_viewerManager || !m_editorManager) {
        return;
    }
    const QString path = m_editorManager->currentFilePath();
    if (!isMarkdownPath(path)) {
        return;
    }
    const ViewerManager::OpenResult result = m_viewerManager->openMarkdownPreview(path);
    if (result == ViewerManager::OpenResult::Opened) {
        ui->centralStack->setCurrentIndex(CodeViewer);
        raise();
        activateWindow();
        syncFileWatches();
        saveSession();
    }
}

void MainWindow::onFileChangedOnDisk(const QString &path)
{
    if (m_scm) {
        m_scm->refresh();
    }
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
        m_editorManager->closeWidget(editor);
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
