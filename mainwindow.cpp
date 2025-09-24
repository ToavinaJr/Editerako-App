#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , codeEditor(nullptr)
    , isModified(false)
    , currentWorkingDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
{
    ui->setupUi(this);

    // Setup the custom code editor
    setupCodeEditor();

    // Connect all actions and signals
    connectActions();

    // Setup file tree
    setupFileTree();

    // Set initial window title
    updateWindowTitle();

    // Set focus on the text editor
    if (codeEditor) {
        codeEditor->setFocus();
    }

    // Load default directory
    loadDirectoryToTree(currentWorkingDirectory);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::connectActions()
{
    // File menu actions
    connect(ui->actionFile, &QAction::triggered, this, &MainWindow::newFile);
    connect(ui->actionNew_Document, &QAction::triggered, this, &MainWindow::newFolder);
    connect(ui->actionOpen_File, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionOpen_Folder, &QAction::triggered, this, &MainWindow::openFolder);

    // Explorer buttons
    connect(ui->addFileButton, &QPushButton::clicked, this, &MainWindow::onAddFileClicked);
    connect(ui->newFolderButton, &QPushButton::clicked, this, &MainWindow::onNewFolderClicked);
    connect(ui->closeExplorerButton, &QPushButton::clicked, this, &MainWindow::onCloseExplorerClicked);

    // File tree interaction
    connect(ui->fileTreeWidget, &QTreeWidget::itemClicked, this, &MainWindow::onFileTreeItemClicked);
    connect(ui->fileTreeWidget, &QTreeWidget::itemDoubleClicked, this, &MainWindow::onFileTreeItemDoubleClicked);

    // Line numbers checkbox
    connect(ui->checkBox, &QCheckBox::toggled, this, &MainWindow::onShowLinesToggled);

    // Text editor modification tracking
    if (codeEditor) {
        connect(codeEditor, &CodeEditor::textChanged, [this]() {
            isModified = true;
            updateWindowTitle();
        });
    }
}

void MainWindow::setupCodeEditor()
{
    // Create custom code editor
    codeEditor = new CodeEditor(this);

    // Apply same styling as the original plainTextEdit
    codeEditor->setStyleSheet(
        "CodeEditor {"
        "    background-color: #1e1e1e;"
        "    color: #cccccc;"
        "    border: none;"
        "    font-family: \"Monaco\", \"Consolas\", monospace;"
        "    font-size: 13px;"
        "    line-height: 1.4;"
        "}"
        );

    // Replace the original plainTextEdit with our custom editor
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(ui->verticalLayout);
    if (layout) {
        // Remove the original plainTextEdit and replace it
        layout->removeWidget(ui->plainTextEdit);
        ui->plainTextEdit->hide();
        layout->insertWidget(0, codeEditor);
    }

    // Set initial state of line numbers based on checkbox
    codeEditor->setLineNumbersVisible(ui->checkBox->isChecked());
}

void MainWindow::setupFileTree()
{
    ui->fileTreeWidget->setHeaderHidden(true);
    ui->fileTreeWidget->setRootIsDecorated(true);
    ui->fileTreeWidget->setAlternatingRowColors(false);

    // Set custom context menu
    ui->fileTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
}

void MainWindow::loadDirectoryToTree(const QString &path)
{
    ui->fileTreeWidget->clear();

    QDir dir(path);
    if (!dir.exists()) {
        return;
    }

    currentWorkingDirectory = path;

    // Create root item
    QTreeWidgetItem *rootItem = new QTreeWidgetItem(ui->fileTreeWidget);
    rootItem->setText(0, QString("📁 %1").arg(dir.dirName()));
    rootItem->setData(0, Qt::UserRole, path);
    rootItem->setExpanded(true);

    // Add directories first
    QStringList dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    foreach(const QString &dirName, dirs) {
        addFolderToTree(dirName, rootItem);
    }

    // Add files
    QStringList files = dir.entryList(QDir::Files, QDir::Name);
    foreach(const QString &fileName, files) {
        addFileToTree(fileName, rootItem);
    }
}

void MainWindow::addFileToTree(const QString &fileName, QTreeWidgetItem *parent)
{
    QTreeWidgetItem *fileItem;
    if (parent) {
        fileItem = new QTreeWidgetItem(parent);
    } else {
        fileItem = new QTreeWidgetItem(ui->fileTreeWidget);
    }

    QString icon = getFileIcon(fileName);
    fileItem->setText(0, QString("%1 %2").arg(icon, fileName));

    QString fullPath;
    if (parent) {
        QString parentPath = parent->data(0, Qt::UserRole).toString();
        fullPath = QDir(parentPath).absoluteFilePath(fileName);
    } else {
        fullPath = QDir(currentWorkingDirectory).absoluteFilePath(fileName);
    }
    fileItem->setData(0, Qt::UserRole, fullPath);
}

void MainWindow::addFolderToTree(const QString &folderName, QTreeWidgetItem *parent)
{
    QTreeWidgetItem *folderItem;
    if (parent) {
        folderItem = new QTreeWidgetItem(parent);
    } else {
        folderItem = new QTreeWidgetItem(ui->fileTreeWidget);
    }

    folderItem->setText(0, QString("📁 %1").arg(folderName));

    QString fullPath;
    if (parent) {
        QString parentPath = parent->data(0, Qt::UserRole).toString();
        fullPath = QDir(parentPath).absoluteFilePath(folderName);
    } else {
        fullPath = QDir(currentWorkingDirectory).absoluteFilePath(folderName);
    }
    folderItem->setData(0, Qt::UserRole, fullPath);

    // Add subdirectories and files
    QDir subDir(fullPath);
    if (subDir.exists()) {
        QStringList subDirs = subDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        foreach(const QString &subDirName, subDirs) {
            addFolderToTree(subDirName, folderItem);
        }

        QStringList files = subDir.entryList(QDir::Files, QDir::Name);
        foreach(const QString &fileName, files) {
            addFileToTree(fileName, folderItem);
        }
    }
}

QString MainWindow::getFileIcon(const QString &fileName)
{
    QString ext = getFileExtension(fileName).toLower();

    if (ext == "cpp" || ext == "cxx" || ext == "cc" || ext == "c") {
        return "🔵";
    } else if (ext == "h" || ext == "hpp" || ext == "hxx") {
        return "🟦";
    } else if (ext == "py") {
        return "🐍";
    } else if (ext == "js") {
        return "🟨";
    } else if (ext == "html" || ext == "htm") {
        return "🌐";
    } else if (ext == "css") {
        return "🎨";
    } else if (ext == "php") {
        return "🐘";
    } else if (ext == "txt") {
        return "📝";
    } else if (ext == "json") {
        return "📋";
    } else if (ext == "xml" || ext == "ui") {
        return "📄";
    } else if (ext == "exe" || ext == "bin") {
        return "⚙️";
    } else {
        return "📄";
    }
}

QString MainWindow::getFileExtension(const QString &fileName)
{
    QFileInfo fileInfo(fileName);
    return fileInfo.suffix();
}

void MainWindow::newFile()
{
    if (askToSaveChanges()) {
        bool ok;
        QString fileName = QInputDialog::getText(this,
                                                 tr("New File"),
                                                 tr("Enter file name:"),
                                                 QLineEdit::Normal,
                                                 tr("untitled.txt"), &ok);

        if (ok && !fileName.isEmpty()) {
            // Create new file in current directory
            QString fullPath = QDir(currentWorkingDirectory).absoluteFilePath(fileName);
            QFile file(fullPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.close();

                // Refresh tree view
                loadDirectoryToTree(currentWorkingDirectory);

                // Open the file in editor
                openFileInEditor(fullPath);

                QMessageBox::information(this, tr("Success"), tr("File created successfully!"));
            } else {
                QMessageBox::warning(this, tr("Error"), tr("Could not create file!"));
            }
        }
    }
}

void MainWindow::newFolder()
{
    bool ok;
    QString folderName = QInputDialog::getText(this,
                                               tr("New Folder"),
                                               tr("Enter folder name:"),
                                               QLineEdit::Normal,
                                               tr("New Folder"), &ok);

    if (ok && !folderName.isEmpty()) {
        QString fullPath = QDir(currentWorkingDirectory).absoluteFilePath(folderName);
        QDir dir;
        if (dir.mkpath(fullPath)) {
            loadDirectoryToTree(currentWorkingDirectory);
            QMessageBox::information(this, tr("Success"), tr("Folder created successfully!"));
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Could not create folder!"));
        }
    }
}

void MainWindow::openFile()
{
    if (askToSaveChanges()) {
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("Open File"),
                                                        currentWorkingDirectory,
                                                        tr("All Files (*.*);;Text Files (*.txt);;C++ Files (*.cpp *.h);;Python Files (*.py)"));

        if (!fileName.isEmpty()) {
            openFileInEditor(fileName);
        }
    }
}

void MainWindow::openFolder()
{
    QString folderPath = QFileDialog::getExistingDirectory(this,
                                                           tr("Open Folder"),
                                                           currentWorkingDirectory);

    if (!folderPath.isEmpty()) {
        loadDirectoryToTree(folderPath);
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
    ui->leftSidebar->setVisible(!ui->leftSidebar->isVisible());
}

void MainWindow::onFileTreeItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)

    if (item) {
        QString filePath = item->data(0, Qt::UserRole).toString();
        QFileInfo fileInfo(filePath);

        if (fileInfo.isFile()) {
            // Update status bar or do something when file is selected
            if (statusBar()) {
                statusBar()->showMessage(QString("Selected: %1").arg(fileInfo.fileName()), 2000);
            }
        }
    }
}

void MainWindow::onFileTreeItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)

    if (item) {
        QString filePath = item->data(0, Qt::UserRole).toString();
        QFileInfo fileInfo(filePath);

        if (fileInfo.isFile()) {
            if (askToSaveChanges()) {
                openFileInEditor(filePath);
            }
        } else if (fileInfo.isDir()) {
            // Toggle folder expansion
            item->setExpanded(!item->isExpanded());
        }
    }
}

void MainWindow::onShowLinesToggled(bool checked)
{
    if (codeEditor) {
        codeEditor->setLineNumbersVisible(checked);
    }
}

void MainWindow::openFileInEditor(const QString &filePath)
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString content = in.readAll();

        if (codeEditor) {
            codeEditor->setPlainText(content);
        }

        currentFileName = filePath;
        isModified = false;
        updateWindowTitle();

        if (statusBar()) {
            statusBar()->showMessage(QString("Opened: %1").arg(QFileInfo(filePath).fileName()), 3000);
        }
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Could not open file: %1").arg(filePath));
    }
}

void MainWindow::saveCurrentFile()
{
    if (currentFileName.isEmpty()) {
        QString fileName = QFileDialog::getSaveFileName(this,
                                                        tr("Save File"),
                                                        currentWorkingDirectory,
                                                        tr("All Files (*.*)"));
        if (fileName.isEmpty()) {
            return;
        }
        currentFileName = fileName;
    }

    QFile file(currentFileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        if (codeEditor) {
            out << codeEditor->toPlainText();
        }

        isModified = false;
        updateWindowTitle();

        // Refresh tree if file is in current directory
        QFileInfo fileInfo(currentFileName);
        if (fileInfo.absolutePath() == currentWorkingDirectory) {
            loadDirectoryToTree(currentWorkingDirectory);
        }

        if (statusBar()) {
            statusBar()->showMessage(tr("File saved successfully"), 2000);
        }
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Could not save file!"));
    }
}

bool MainWindow::askToSaveChanges()
{
    if (isModified) {
        QMessageBox::StandardButton result = QMessageBox::question(this,
                                                                   tr("Save Changes"),
                                                                   tr("The document has been modified. Do you want to save your changes?"),
                                                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (result == QMessageBox::Save) {
            saveCurrentFile();
            return true;
        } else if (result == QMessageBox::Cancel) {
            return false;
        }
    }
    return true;
}

void MainWindow::updateWindowTitle()
{
    QString title = "Code Editor";

    if (!currentFileName.isEmpty()) {
        QFileInfo fileInfo(currentFileName);
        title += " - " + fileInfo.fileName();

        if (isModified) {
            title += "*";
        }
    }

    setWindowTitle(title);
}
