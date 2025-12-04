#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "syntaxhighlighter.h"
#include "finddialog.h"
#include "chatwidget.h"
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
    , editorTabs(nullptr)
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

    // Setup the tabbed code editor area
    setupCodeEditor();

    // Connect all actions and signals
    connectActions();

    // Setup file tree
    setupFileTree();

    // Set initial window title
    updateWindowTitle();

    // Set focus on the active text editor
    if (currentEditor()) {
        currentEditor()->setFocus();
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

    // Replace right sidebar placeholder with ChatWidget (Gemini chat)
    chatWidget = new ChatWidget(this);
    if (ui->rightChatPlaceholder) {
        // parent the chat widget into the placeholder's parent layout
        QWidget *ph = ui->rightChatPlaceholder;
        QLayout *parentLayout = ph->parentWidget() ? ph->parentWidget()->layout() : nullptr;
        if (parentLayout) {
            // find the index of placeholder and replace it
            for (int i = 0; i < parentLayout->count(); ++i) {
                QLayoutItem *item = parentLayout->itemAt(i);
                if (item && item->widget() == ph) {
                    // remove placeholder
                    QLayoutItem *removed = parentLayout->takeAt(i);
                    if (removed) {
                        if (removed->widget()) removed->widget()->deleteLater();
                        delete removed;
                    }
                    // Try to insert at same index if the layout is a box layout
                    QBoxLayout *box = qobject_cast<QBoxLayout*>(parentLayout);
                    if (box) {
                        box->insertWidget(i, chatWidget);
                    } else {
                        parentLayout->addWidget(chatWidget);
                    }
                    break;
                }
            }
        } else {
            // fallback: add to rightSidebar layout
            if (ui->rightSidebar && ui->rightSidebar->layout()) {
                ui->rightSidebar->layout()->addWidget(chatWidget);
            } else if (ui->rightSidebar) {
                QVBoxLayout *newLayout = new QVBoxLayout(ui->rightSidebar);
                newLayout->setContentsMargins(6,6,6,6);
                newLayout->addWidget(chatWidget);
            }
        }
    } else if (ui->rightSidebar) {
        if (ui->rightSidebar->layout()) ui->rightSidebar->layout()->addWidget(chatWidget);
        else {
            QVBoxLayout *newLayout = new QVBoxLayout(ui->rightSidebar);
            newLayout->setContentsMargins(6,6,6,6);
            newLayout->addWidget(chatWidget);
        }
    }
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

    // Text editor modification tracking will be connected per-tab when created

    // Connection to the find dialog
    connect(ui->actionFindReplace, &QAction::triggered, this, &MainWindow::onActionFindReplace);
}

void MainWindow::setupCodeEditor()
{
    // Create the tab widget that will hold multiple CodeEditor instances
    editorTabs = new QTabWidget(this);
    editorTabs->setTabsClosable(true);
    editorTabs->setMovable(true);

    // Replace the old central widget at CodeViewer with the tab widget
    QWidget *oldEditor = ui->centralStack->widget(CodeViewer);
    if (oldEditor) {
        ui->centralStack->removeWidget(oldEditor);
        oldEditor->deleteLater();
    }

    ui->centralStack->insertWidget(CodeViewer, editorTabs);
    ui->centralStack->setCurrentIndex(CodeViewer);

    // Create an initial untitled editor tab
    CodeEditor *initial = new CodeEditor(this);
    initial->setStyleSheet(
        "background-color: #1e1e1e;"
        "color: #cccccc;"
        "border: none;"
        "font-family: 'Monaco', 'Consolas', monospace;"
        "font-size: 13px;"
        );
    editorTabs->addTab(initial, tr("untitled"));
    initial->setProperty("filePath", QString());

    // Syntax highlighter for the new editor
    new SyntaxHighlighter(initial, SyntaxHighlighter::CPP);

    connect(initial, &CodeEditor::textChanged, [this, initial](){
        updateTabModifiedState(initial);
    });

    connect(editorTabs, &QTabWidget::currentChanged, this, &MainWindow::onEditorTabChanged);
    connect(editorTabs, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
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
    if (currentEditor()) {
        currentEditor()->setLineNumbersVisible(checked);
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

            // If a tab for this file already exists, switch to it
            for (int i = 0; i < editorTabs->count(); ++i) {
                QWidget *w = editorTabs->widget(i);
                if (w && w->property("filePath").toString() == filePath) {
                    editorTabs->setCurrentIndex(i);
                    return;
                }
            }

            // Create a new editor tab and load content
            CodeEditor *ed = new CodeEditor(this);
            ed->setPlainText(content);
            ed->setStyleSheet(
                "background-color: #1e1e1e;"
                "color: #cccccc;"
                "border: none;"
                "font-family: 'Monaco', 'Consolas', monospace;"
                "font-size: 13px;"
                );

            editorTabs->addTab(ed, QFileInfo(filePath).fileName());
            editorTabs->setCurrentWidget(ed);
            ed->setProperty("filePath", filePath);

            // Highlighter selon extension
            new SyntaxHighlighter(ed,
                                   (ext == "cpp") ? SyntaxHighlighter::CPP :
                                   (ext == "html") ? SyntaxHighlighter::HTML :
                                   (ext == "tsx") ? SyntaxHighlighter::HTML : SyntaxHighlighter::CPP
                                   );

            // Mark as unmodified after loading
            ed->document()->setModified(false);
            updateTabLabel(ed);

            connect(ed, &CodeEditor::textChanged, [this, ed](){
                updateTabModifiedState(ed);
            });

            currentFileName = filePath;
            isModified = false;
            updateWindowTitle();
            ui->centralStack->setCurrentIndex(CodeViewer);
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
    CodeEditor *ed = currentEditor();
    if (!ed) return;

    QString path = ed->property("filePath").toString();
    if (path.isEmpty()) {
        QString fileName = QFileDialog::getSaveFileName(this,
                                                        tr("Save File"),
                                                        currentWorkingDirectory,
                                                        tr("All Files (*.*)"));
        if (fileName.isEmpty()) {
            return;
        }
        path = fileName;
        ed->setProperty("filePath", path);
        int idx = editorTabs ? editorTabs->indexOf(ed) : -1;
        if (idx >= 0) editorTabs->setTabText(idx, QFileInfo(path).fileName());
    }

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << ed->toPlainText();

        ed->document()->setModified(false);
        updateTabModifiedState(ed);

        currentFileName = path;
        isModified = false;
        updateWindowTitle();

        QFileInfo fileInfo(path);
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

CodeEditor *MainWindow::currentEditor()
{
    if (!editorTabs) return nullptr;
    QWidget *w = editorTabs->currentWidget();
    return qobject_cast<CodeEditor*>(w);
}

void MainWindow::onEditorTabChanged(int index)
{
    Q_UNUSED(index)
    if (!editorTabs) return;
    QWidget *w = editorTabs->currentWidget();
    if (w) {
        currentFileName = w->property("filePath").toString();
    } else {
        currentFileName.clear();
    }
    updateWindowTitle();
}

bool MainWindow::saveEditor(CodeEditor *editor)
{
    if (!editor) return false;
    QString path = editor->property("filePath").toString();
    if (path.isEmpty()) {
        QString fileName = QFileDialog::getSaveFileName(this,
                                                      tr("Save File"),
                                                      currentWorkingDirectory,
                                                      tr("All Files (*.*)"));
        if (fileName.isEmpty()) return false; // user cancelled
        path = fileName;
        editor->setProperty("filePath", path);
        int idx = editorTabs->indexOf(editor);
        if (idx >= 0) editorTabs->setTabText(idx, QFileInfo(path).fileName());
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not save file!"));
        return false;
    }
    QTextStream out(&file);
    out << editor->toPlainText();
    file.close();

    editor->document()->setModified(false);
    updateTabModifiedState(editor);

    currentFileName = path;
    isModified = false;
    updateWindowTitle();

    QFileInfo fileInfo(path);
    if (fileInfo.absolutePath() == currentWorkingDirectory) {
        loadDirectoryToTree(currentWorkingDirectory);
    }

    if (statusBar()) {
        statusBar()->showMessage(tr("File saved successfully"), 2000);
    }

    return true;
}

void MainWindow::updateTabModifiedState(CodeEditor *editor)
{
    if (!editor || !editorTabs) return;
    bool modified = editor->document()->isModified();
    editor->setProperty("fileModified", modified);
    updateTabLabel(editor);

    if (editor == currentEditor()) {
        isModified = modified;
        updateWindowTitle();
    }
}

void MainWindow::updateTabLabel(CodeEditor *editor)
{
    if (!editor || !editorTabs) return;
    int idx = editorTabs->indexOf(editor);
    if (idx < 0) return;
    QString path = editor->property("filePath").toString();
    QString label = path.isEmpty() ? tr("untitled") : QFileInfo(path).fileName();
    if (editor->document()->isModified()) label += "*";
    editorTabs->setTabText(idx, label);
    editorTabs->setTabToolTip(idx, path);
}

void MainWindow::closeTab(int index)
{
    if (!editorTabs) return;
    QWidget *w = editorTabs->widget(index);
    CodeEditor *ed = qobject_cast<CodeEditor*>(w);
    if (!ed) {
        editorTabs->removeTab(index);
        if (w) w->deleteLater();
        return;
    }

    if (ed->document()->isModified()) {
        QMessageBox msg(this);
        msg.setWindowTitle(tr("Save Changes"));
        msg.setText(tr("The document has been modified. Do you want to save your changes?"));
        msg.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msg.setDefaultButton(QMessageBox::Save);
        int res = msg.exec();
        if (res == QMessageBox::Save) {
            if (!saveEditor(ed)) return; // save cancelled or failed -> do not close
        } else if (res == QMessageBox::Cancel) {
            return; // do not close
        }
        // if Discard, continue to close
    }

    editorTabs->removeTab(index);
    if (w) w->deleteLater();
}

void MainWindow::onActionFindReplace() {
    FindReplaceDialog dlg(currentEditor(), this);
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
    if (currentEditor()) {
        currentEditor()->setFocus();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Check if any tabs have unsaved changes
    QList<CodeEditor*> modifiedEditors;
    if (editorTabs) {
        for (int i = 0; i < editorTabs->count(); ++i) {
            CodeEditor *ed = qobject_cast<CodeEditor*>(editorTabs->widget(i));
            if (ed && ed->document()->isModified()) {
                modifiedEditors.append(ed);
            }
        }
    }

    if (modifiedEditors.isEmpty()) {
        event->accept();
        return;
    }

    // Ask user what to do with unsaved changes
    QMessageBox msg(this);
    msg.setWindowTitle(tr("Unsaved Changes"));
    msg.setText(tr("You have %1 file(s) with unsaved changes.\nDo you want to save all changes before closing?").arg(modifiedEditors.size()));
    msg.setStandardButtons(QMessageBox::SaveAll | QMessageBox::Discard | QMessageBox::Cancel);
    msg.setDefaultButton(QMessageBox::SaveAll);
    msg.setIcon(QMessageBox::Warning);

    int res = msg.exec();

    if (res == QMessageBox::SaveAll) {
        // Save all modified editors
        for (CodeEditor *ed : modifiedEditors) {
            if (!saveEditor(ed)) {
                // Save was cancelled or failed — abort close
                event->ignore();
                return;
            }
        }
        event->accept();
    } else if (res == QMessageBox::Discard) {
        // Discard all changes and close
        event->accept();
    } else {
        // Cancel — do not close the application
        event->ignore();
    }
}
