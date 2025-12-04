#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "syntaxhighlighter.h"
#include "finddialog.h"
#include <QApplication>
#include <QStandardPaths>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include <QFileInfo>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , codeEditor(nullptr)
    , isModified(false)
    , currentWorkingDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
{

    ui->setupUi(this);

    pdfDoc = new QPdfDocument(this);
    pdfView = new QPdfView(this);
    pdfView->setDocument(pdfDoc);
    pdfView->setZoomFactor(1.0);

    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageScroll = new QScrollArea(this);
    imageScroll->setWidget(imageLabel);
    imageScroll->setWidgetResizable(true);

    // Ajout dans la pile
    ui->centralStack->insertWidget(PdfViewer, pdfView);
    ui->centralStack->insertWidget(ImageViewer, imageScroll);
    ui->centralStack->setCurrentIndex(CodeViewer); // par défaut

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

    // Adding shortcut
    ui->actionFindReplace->setShortcut(QKeySequence("Ctrl+F"));

    // Setup terminal
    terminal = new Terminal(this);
    terminal->setVisible(false);
    isTerminalVisible = false;

    // Add terminal below the central code viewer (use `verticalLayout` from ui)
    if (ui->verticalLayout) {
        ui->verticalLayout->addWidget(terminal);
        terminal->setMinimumHeight(250);
        terminal->setMaximumHeight(400);
    } else {
        // Fallback: parent to central widget so it shows (not managed by layout)
        terminal->setParent(ui->centralwidget);
        terminal->setMinimumHeight(250);
        terminal->setMaximumHeight(400);
    }

    // Setup Ctrl+J shortcut for terminal
    terminalShortcut = new QShortcut(QKeySequence("Ctrl+J"), this);
    connect(terminalShortcut, &QShortcut::activated, this, &MainWindow::toggleTerminal);

    // Connect terminal closed signal
    connect(terminal, &Terminal::terminalClosed, this, &MainWindow::onTerminalClosed);
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

    // Connection to the find dialog
    connect(ui->actionFindReplace, &QAction::triggered, this, &MainWindow::onActionFindReplace);
}

void MainWindow::setupCodeEditor()
{
    // Crée le code editor
    codeEditor = new CodeEditor(this);
    codeEditor->setStyleSheet(
        "background-color: #1e1e1e;"
        "color: #cccccc;"
        "border: none;"
        "font-family: 'Monaco', 'Consolas', monospace;"
        "font-size: 13px;"
        );

    // Remplacer l'ancien QPlainTextEdit
    QWidget *oldEditor = ui->centralStack->widget(CodeViewer);
    if (oldEditor) {
        ui->centralStack->removeWidget(oldEditor);
        oldEditor->deleteLater();
    }

    ui->centralStack->insertWidget(CodeViewer, codeEditor);
    ui->centralStack->setCurrentIndex(CodeViewer);

    // Highlighter syntaxique
    SyntaxHighlighter *highlighter = new SyntaxHighlighter(codeEditor, SyntaxHighlighter::CPP);

    // Connexion du suivi de modification
    connect(codeEditor, &CodeEditor::textChanged, [this](){
        isModified = true;
        updateWindowTitle();
    });
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

        // Create styled input dialog
        QInputDialog dialog(this);
        dialog.setWindowTitle(tr("New File"));
        dialog.setLabelText(tr("Enter file name:"));
        dialog.setTextValue(tr("untitled.txt"));
        dialog.setInputMode(QInputDialog::TextInput);

        // Set minimum size for the dialog
        dialog.setMinimumWidth(800);
        dialog.setMinimumHeight(150);

        // Apply dark theme stylesheet
        dialog.setStyleSheet(
            "QInputDialog {"
            "    background-color: #1e1e1e;"
            "    color: #cccccc;"
            "}"
            "QLabel {"
            "    color: #cccccc;"
            "    font-size: 12px;"
            "}"
            "QLineEdit {"
            "    background-color: #3e3e42;"
            "    border: 1px solid #6f6f6f;"
            "    border-radius: 4px;"
            "    color: #cccccc;"
            "    padding: 8px;"
            "    font-size: 12px;"
            "}"
            "QLineEdit:focus {"
            "    border: 1px solid #98c379;"
            "}"
            "QPushButton {"
            "    background-color: #3e3e42;"
            "    border: 1px solid #6f6f6f;"
            "    border-radius: 4px;"
            "    color: #cccccc;"
            "    padding: 6px 16px;"
            "    font-size: 11px;"
            "    min-width: 60px;"
            "}"
            "QPushButton:hover {"
            "    background-color: #6f6f6f;"
            "}"
            "QPushButton:pressed {"
            "    background-color: #98c379;"
            "    color: #1e1e1e;"
            "}"
            );

        ok = dialog.exec();
        QString fileName = dialog.textValue();

        if (ok && !fileName.isEmpty()) {
            QString fullPath = QDir(currentWorkingDirectory).absoluteFilePath(fileName);
            QFile file(fullPath);

            if (file.open(QIODevice::WriteOnly)) {
                file.close();
                loadDirectoryToTree(currentWorkingDirectory);
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

    // Create styled input dialog
    QInputDialog dialog(this);
    dialog.setWindowTitle(tr("New Folder"));
    dialog.setLabelText(tr("Enter folder name:"));
    dialog.setTextValue(tr("New Folder"));
    dialog.setInputMode(QInputDialog::TextInput);

    // Set minimum size for the dialog
    dialog.setMinimumWidth(400);
    dialog.setMinimumHeight(150);
    // Apply dark theme stylesheet
    dialog.setStyleSheet(
        "QInputDialog {"
        "    background-color: #1e1e1e;"
        "    color: #cccccc;"
        "}"
        "QLabel {"
        "    color: #cccccc;"
        "    font-size: 12px;"
        "}"
        "QLineEdit {"
        "    background-color: #3e3e42;"
        "    border: 1px solid #6f6f6f;"
        "    border-radius: 4px;"
        "    color: #cccccc;"
        "    padding: 8px;"
        "    font-size: 12px;"
        "}"
        "QLineEdit:focus {"
        "    border: 1px solid #98c379;"
        "}"
        "QPushButton {"
        "    background-color: #3e3e42;"
        "    border: 1px solid #6f6f6f;"
        "    border-radius: 4px;"
        "    color: #cccccc;"
        "    padding: 6px 16px;"
        "    font-size: 11px;"
        "    min-width: 60px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #6f6f6f;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #98c379;"
        "    color: #1e1e1e;"
        "}"
        );

    ok = dialog.exec();
    QString folderName = dialog.textValue();

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
        if (item->childCount() > 0) {
            // Si c’est un dossier, on inverse son état
            item->setExpanded(!item->isExpanded());
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
    QFileInfo info(filePath);
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(info);
    QString mimeName = mime.name();

    QString ext = info.suffix().toLower();

    if (mimeName.startsWith("text/") || mimeName.contains("json") || mimeName.contains("xml") || mimeName.contains("html") || ext == "tsx") {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();

            codeEditor->setPlainText(content);
            ui->centralStack->setCurrentIndex(CodeViewer);

            // Highlighter selon extension
            SyntaxHighlighter *highlighter = new SyntaxHighlighter(codeEditor,
                                                                   (ext == "cpp") ? SyntaxHighlighter::CPP :
                                                                       (ext == "html") ? SyntaxHighlighter::HTML :
                                                                       (ext == "tsx") ? SyntaxHighlighter::HTML : SyntaxHighlighter::CPP
                                                                   );

            currentFileName = filePath;
            isModified = false;
            updateWindowTitle();
        }
    }

    else if (mimeName == "application/pdf") {
        pdfDoc->load(filePath);
        ui->centralStack->setCurrentIndex(PdfViewer);
    }
    else if (mimeName.startsWith("image/")) {
        QPixmap pix(filePath);
        imageLabel->setPixmap(pix); // taille réelle
        ui->centralStack->setCurrentIndex(ImageViewer);
    }
    else {
        ui->centralStack->setCurrentIndex(UnsupportedViewer);
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

void MainWindow::onActionFindReplace() {
    FindReplaceDialog dlg(codeEditor, this);
    dlg.exec();
}

void MainWindow::toggleTerminal()
{
    isTerminalVisible = !isTerminalVisible;
    terminal->setVisible(isTerminalVisible);

    if (isTerminalVisible) {
        // Set terminal working directory to current file's directory or project directory
        if (!currentFileName.isEmpty()) {
            QFileInfo fileInfo(currentFileName);
            terminal->setWorkingDirectory(fileInfo.absolutePath());
        } else {
            terminal->setWorkingDirectory(currentWorkingDirectory);
        }
        terminal->focusTerminal();
    }
}

void MainWindow::onTerminalClosed()
{
    isTerminalVisible = false;
    terminal->setVisible(false);

    // Return focus to code editor
    if (codeEditor) {
        codeEditor->setFocus();
    }
}
