#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "finddialog.h"
#include "gotolinedialog.h"
#include "chatwidget.h"
#include "editor/EditorDocument.h"
#include "editor/EditorManager.h"
#include <QApplication>
#include <QStandardPaths>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include <QFileInfo>
#include <QTextStream>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_editorManager(nullptr)
    , currentWorkingDirectory()
    , isFileTreeVisible(true)
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
    ui->centralStack->setCurrentIndex(CodeViewer);

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

    // Ask user to select a folder/file to open at startup
    promptOpenFolderOrFile();

    // Adding shortcut
    ui->actionFindReplace->setShortcut(QKeySequence("Ctrl+F"));

    // Setup terminal tabs system
    setupTerminalTabs();

    // Replace right sidebar placeholder with ChatWidget (Gemini chat)
    chatWidget = new ChatWidget(this);
    // Set project directory for chat history (will be updated when folder changes)
    if (!currentWorkingDirectory.isEmpty()) {
        chatWidget->setProjectDirectory(currentWorkingDirectory);
    }
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

    // Enable drag and drop for opening files
    setAcceptDrops(true);
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
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveCurrentDocument);
    connect(ui->actionSave_As, &QAction::triggered, this, &MainWindow::saveCurrentDocumentAs);
    connect(ui->actionSave_All, &QAction::triggered, this, &MainWindow::saveAllDocuments);
    connect(ui->actionClose, &QAction::triggered, this, &MainWindow::closeCurrentTab);
    connect(ui->actionClose_Others, &QAction::triggered, this, &MainWindow::closeOtherTabs);
    connect(ui->actionClose_All, &QAction::triggered, this, &MainWindow::closeAllTabs);

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
    connect(ui->actionGoToLine, &QAction::triggered, this, &MainWindow::onActionGoToLine);
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

    connect(m_editorManager, &EditorManager::currentChanged, this, &MainWindow::updateWindowTitle);
    connect(m_editorManager, &EditorManager::modificationChanged, this, &MainWindow::updateWindowTitle);
    connect(m_editorManager, &EditorManager::fileSaved, this, [this](const QString &path) {
        const QFileInfo info(path);
        if (info.absolutePath() == currentWorkingDirectory) {
            loadDirectoryToTree(currentWorkingDirectory);
        }
        if (statusBar()) {
            statusBar()->showMessage(tr("File saved successfully"), 2000);
        }
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
    bool ok;

    QInputDialog dialog(this);
    dialog.setWindowTitle(tr("New File"));
    dialog.setLabelText(tr("Enter file name:"));
    dialog.setTextValue(tr("untitled.txt"));
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setMinimumWidth(800);
    dialog.setMinimumHeight(150);
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
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open File"),
                                                    currentWorkingDirectory,
                                                    tr("All Files (*.*);;Text Files (*.txt);;C++ Files (*.cpp *.h);;Python Files (*.py)"));

    if (!fileName.isEmpty()) {
        openFileInEditor(fileName);
    }
}

void MainWindow::openFolder()
{
    QString folderPath = QFileDialog::getExistingDirectory(this,
                                                           tr("Open Folder"),
                                                           currentWorkingDirectory.isEmpty() 
                                                               ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                                                               : currentWorkingDirectory);

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
            openFileInEditor(filePath);
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
        if (m_editorManager->openTextFile(filePath)) {
            ui->centralStack->setCurrentIndex(CodeViewer);
        }
    }

    else if (mimeName == "application/pdf") {
        if (m_editorManager->activateExisting(filePath)) {
            return;
        }

        QWidget *container = new QWidget(this);
        QVBoxLayout *lay = new QVBoxLayout(container);
        lay->setContentsMargins(0,0,0,0);

        QPdfDocument *doc = new QPdfDocument(container);
        doc->load(filePath);
        if (doc->pageCount() > 0) {
            QPdfView *pv = new QPdfView(container);
            pv->setDocument(doc);
            pv->setZoomFactor(1.0);
            pv->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            lay->addWidget(pv);

            container->setProperty("viewerType", "pdf");
            m_editorManager->addViewerTab(container, filePath);
            this->raise(); this->activateWindow();
        } else {
            delete container;
            pdfDoc->load(filePath);
            ui->centralStack->setCurrentIndex(PdfViewer);
        }
    }
    else if (mimeName.startsWith("image/")) {
        if (m_editorManager->activateExisting(filePath)) {
            return;
        }

        QWidget *container = new QWidget(this);
        QVBoxLayout *lay = new QVBoxLayout(container);
        lay->setContentsMargins(0,0,0,0);

        QLabel *lbl = new QLabel(container);
        lbl->setAlignment(Qt::AlignCenter);
        QPixmap pix(filePath);
        lbl->setPixmap(pix);
        lbl->setScaledContents(true);

        QScrollArea *scroll = new QScrollArea(container);
        scroll->setWidget(lbl);
        scroll->setWidgetResizable(true);
        lay->addWidget(scroll);

        container->setProperty("viewerType", "image");
        m_editorManager->addViewerTab(container, filePath);
        this->raise(); this->activateWindow();
    }
    else {
        ui->centralStack->setCurrentIndex(UnsupportedViewer);
    }
}

void MainWindow::saveCurrentDocument()
{
    if (m_editorManager) {
        m_editorManager->saveCurrent();
    }
}

void MainWindow::saveCurrentDocumentAs()
{
    if (m_editorManager) {
        m_editorManager->saveCurrentAs();
    }
}

void MainWindow::saveAllDocuments()
{
    if (m_editorManager) {
        m_editorManager->saveAll();
    }
}

void MainWindow::closeCurrentTab()
{
    if (m_editorManager) {
        m_editorManager->closeCurrent();
    }
}

void MainWindow::closeOtherTabs()
{
    if (m_editorManager) {
        m_editorManager->closeOthers();
    }
}

void MainWindow::closeAllTabs()
{
    if (m_editorManager) {
        m_editorManager->closeAll();
    }
}

void MainWindow::updateWindowTitle()
{
    QString title = QStringLiteral("Editerako");

    if (m_editorManager) {
        const QString path = m_editorManager->currentFilePath();
        if (!path.isEmpty()) {
            title += QStringLiteral(" - ") + QFileInfo(path).fileName();
        } else if (m_editorManager->currentEditor()) {
            title += QStringLiteral(" - ") + tr("untitled");
        }

        if (auto *doc = m_editorManager->currentDocument(); doc && doc->isModified()) {
            title += QLatin1Char('*');
        }
    }

    setWindowTitle(title);
}

CodeEditor *MainWindow::currentEditor()
{
    return m_editorManager ? m_editorManager->currentEditor() : nullptr;
}

QString MainWindow::editorDirectoryOrWorkspace() const
{
    if (m_editorManager) {
        const QString path = m_editorManager->currentFilePath();
        if (!path.isEmpty()) {
            return QFileInfo(path).absolutePath();
        }
    }
    return currentWorkingDirectory;
}

void MainWindow::onActionFindReplace() {
    CodeEditor *ed = currentEditor();
    if (!ed) {
        QMessageBox::information(this, tr("Find"), tr("No text editor is active."));
        return;
    }
    FindReplaceDialog dlg(ed, this);
    dlg.exec();
}

void MainWindow::onActionGoToLine() {
    CodeEditor *ed = currentEditor();
    if (!ed) {
        QMessageBox::information(this, tr("Go to Line"), tr("No text editor is active."));
        return;
    }
    GoToLineDialog dlg(ed, this);
    dlg.exec();
}

void MainWindow::toggleTerminal()
{
    // Trouver le conteneur des terminaux (parent du terminalTabs)
    QWidget *terminalContainer = terminalTabs ? terminalTabs->parentWidget() : nullptr;
    if (terminalContainer) {
        isTerminalVisible = !isTerminalVisible;
        terminalContainer->setVisible(isTerminalVisible);

        if (isTerminalVisible) {
            // Donner le focus au terminal actif
            Terminal *currentTerminal = terminalList.at(terminalTabs->currentIndex());
            if (currentTerminal) {
                // Set terminal working directory to current file's directory or project directory
                currentTerminal->setWorkingDirectory(editorDirectoryOrWorkspace());
                currentTerminal->focusTerminal();
            }
        } else {
            // Retourner le focus à l'éditeur
            if (currentEditor()) {
                currentEditor()->setFocus();
            }
        }
    }
}

void MainWindow::onTerminalClosed()
{
    // Cette méthode peut être supprimée car nous gérons maintenant plusieurs terminaux
    // et la fermeture se fait via les onglets
    // Garder pour compatibilité mais ne rien faire
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_editorManager && !m_editorManager->promptSaveAllOnQuit()) {
        event->ignore();
        return;
    }

    if (chatWidget) {
        chatWidget->saveChatHistory();
    }
    event->accept();
}

void MainWindow::promptOpenFolderOrFile()
{
    // Show a dialog asking user to open a folder or file
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Welcome to Editerako"));
    msgBox.setText(tr("What would you like to open?"));
    msgBox.setIcon(QMessageBox::Question);
    
    QPushButton *folderBtn = msgBox.addButton(tr("Open Folder"), QMessageBox::AcceptRole);
    QPushButton *fileBtn = msgBox.addButton(tr("Open File"), QMessageBox::AcceptRole);
    msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);
    
    msgBox.setStyleSheet(
        "QMessageBox {"
        "    background-color: #1e1e1e;"
        "    color: #cccccc;"
        "}"
        "QLabel {"
        "    color: #cccccc;"
        "    font-size: 14px;"
        "}"
        "QPushButton {"
        "    background-color: #3e3e42;"
        "    border: 1px solid #6f6f6f;"
        "    border-radius: 4px;"
        "    color: #cccccc;"
        "    padding: 8px 16px;"
        "    min-width: 100px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #6f6f6f;"
        "}"
    );
    
    msgBox.exec();
    
    if (msgBox.clickedButton() == folderBtn) {
        QString folderPath = QFileDialog::getExistingDirectory(this,
                                                               tr("Open Folder"),
                                                               QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        if (!folderPath.isEmpty()) {
            setProjectDirectory(folderPath);
            // Restore focus to main window and editor for better UX
            this->raise();
            this->activateWindow();
            if (currentEditor()) currentEditor()->setFocus();
        } else {
            // User cancelled, use Documents as fallback
            setProjectDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
            this->raise();
            this->activateWindow();
            if (currentEditor()) currentEditor()->setFocus();
        }
    } else if (msgBox.clickedButton() == fileBtn) {
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("Open File"),
                                                        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                        tr("All Files (*.*)"));
        if (!fileName.isEmpty()) {
            // Set project directory to the file's parent folder
            QFileInfo fileInfo(fileName);
            setProjectDirectory(fileInfo.absolutePath());
            openFileInEditor(fileName);
            // Restore focus to main window and editor for better UX
            this->raise();
            this->activateWindow();
            if (currentEditor()) currentEditor()->setFocus();
        } else {
            // User cancelled, use Documents as fallback
            setProjectDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
            this->raise();
            this->activateWindow();
            if (currentEditor()) currentEditor()->setFocus();
        }
    } else {
        // Cancel clicked, use Documents as default
        setProjectDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        this->raise();
        this->activateWindow();
        if (currentEditor()) currentEditor()->setFocus();
    }
}

void MainWindow::setProjectDirectory(const QString &path)
{
    currentWorkingDirectory = path;
    if (m_editorManager) {
        m_editorManager->setWorkingDirectory(path);
    }
    loadDirectoryToTree(path);

    // Mettre à jour le répertoire de travail de tous les terminaux
    for (Terminal *terminal : terminalList) {
        terminal->setWorkingDirectory(path);
    }

    // Update chat widget with new project directory
    if (chatWidget) {
        chatWidget->setProjectDirectory(path);
    }

    // Update window title
    updateWindowTitle();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept drag if it contains URLs (files)
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();

        for (const QUrl &url : urlList) {
            QString filePath = url.toLocalFile();

            // Fix for Windows: if toLocalFile() doesn't work properly, use url.path()
#ifdef Q_OS_WIN
            if (filePath.isEmpty() || filePath.startsWith("file://")) {
                QString path = url.path();
                if (path.startsWith('/')) {
                    path = path.mid(1); // Remove leading slash on Windows
                }
                filePath = path;
            }
#endif

            if (!filePath.isEmpty()) {
                QFileInfo fileInfo(filePath);

                if (fileInfo.isFile()) {
                    openFileInEditor(filePath);
                } else if (fileInfo.isDir()) {
                    // If a directory is dropped, set it as the project directory
                    setProjectDirectory(filePath);
                }
            }
        }

        event->acceptProposedAction();
    }
}

void MainWindow::setupTerminalTabs()
{
    // Créer le tab widget pour les terminaux
    terminalTabs = new QTabWidget(this);
    terminalTabs->setMovable(true);

    // Style amélioré pour les terminaux avec boutons de fermeture
    terminalTabs->setStyleSheet(
        "QTabWidget::pane {"
        "    border: 1px solid #3e3e42;"
        "    background-color: #1e1e1e;"
        "}"
        "QTabBar {"
        "    background-color: #2d2d30;"
        "}"
        "QTabBar::tab {"
        "    background-color: #2d2d30;"
        "    color: #969696;"
        "    border: none;"
        "    border-right: 1px solid #3e3e42;"
        "    padding: 6px 12px;"
        "    padding-right: 24px;"
        "    min-width: 80px;"
        "    font-size: 11px;"
        "}"
        "QTabBar::tab:selected {"
        "    background-color: #1e1e1e;"
        "    color: #ffffff;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "    background-color: #2a2d2e;"
        "}"
    );

    // Créer le premier terminal
    Terminal *firstTerminal = new Terminal(this);
    terminalList.append(firstTerminal);
    
    // Connecter le signal de fermeture du premier terminal
    connect(firstTerminal, &Terminal::terminalClosed, this, [this, firstTerminal]() {
        int index = terminalList.indexOf(firstTerminal);
        if (index >= 0) {
            closeTerminalTab(index);
        }
    });

    // Ajouter le premier terminal dans les tabs
    int firstTabIndex = terminalTabs->addTab(firstTerminal, "⚡ Terminal 1");
    
    // Créer un bouton de fermeture personnalisé avec × visible
    QPushButton *closeBtn = new QPushButton("×", this);
    closeBtn->setFixedSize(16, 16);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    border: none;"
        "    border-radius: 3px;"
        "    color: #909090;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    padding: 0px;"
        "    margin: 0px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e06c75;"
        "    color: #ffffff;"
        "}"
    );
    connect(closeBtn, &QPushButton::clicked, this, [this, firstTerminal]() {
        int idx = terminalTabs->indexOf(firstTerminal);
        if (idx >= 0) closeTerminalTab(idx);
    });
    terminalTabs->tabBar()->setTabButton(firstTabIndex, QTabBar::RightSide, closeBtn);

    // Créer le bouton + pour ajouter de nouveaux terminaux
    addTerminalButton = new QPushButton("+", this);
    addTerminalButton->setFixedSize(28, 28);
    addTerminalButton->setToolTip(tr("Add new terminal"));
    addTerminalButton->setCursor(Qt::PointingHandCursor);
    addTerminalButton->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    border: none;"
        "    color: #cccccc;"
        "    font-weight: bold;"
        "    font-size: 16px;"
        "    padding: 0px;"
        "    margin-right: 8px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #3e3e42;"
        "    color: #ffffff;"
        "    border-radius: 3px;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #4a9eff;"
        "    color: #ffffff;"
        "}"
    );

    // Créer un container pour le bouton + avec marge à droite
    QWidget *cornerWidget = new QWidget(this);
    QHBoxLayout *cornerLayout = new QHBoxLayout(cornerWidget);
    cornerLayout->setContentsMargins(0, 0, 15, 0);  // Marge à droite pour éviter le panneau droit
    cornerLayout->setSpacing(0);
    cornerLayout->addWidget(addTerminalButton);
    
    // Placer le container à droite des onglets
    terminalTabs->setCornerWidget(cornerWidget, Qt::TopRightCorner);

    // Créer un widget conteneur pour les tabs
    QWidget *terminalContainer = new QWidget(this);
    QVBoxLayout *containerLayout = new QVBoxLayout(terminalContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(terminalTabs);

    // Ajouter dans le layout vertical
    if (ui->verticalLayout) {
        ui->verticalLayout->addWidget(terminalContainer);
        terminalContainer->setMinimumHeight(250);
        terminalContainer->setMaximumHeight(400);
    }

    // Connecter les signaux
    connect(addTerminalButton, &QPushButton::clicked, this, &MainWindow::addNewTerminal);
    connect(terminalTabs, &QTabWidget::currentChanged, this, &MainWindow::onTerminalTabChanged);

    // Configurer le premier terminal
    firstTerminal->setWorkingDirectory(currentWorkingDirectory);
    firstTerminal->setVisible(true);
    isTerminalVisible = true;

    // Cacher initialement les terminaux
    terminalContainer->setVisible(false);

    // Setup Ctrl+J shortcut for terminal
    terminalShortcut = new QShortcut(QKeySequence("Ctrl+J"), this);
    connect(terminalShortcut, &QShortcut::activated, this, &MainWindow::toggleTerminal);
}

void MainWindow::addNewTerminal()
{
    Terminal *newTerminal = new Terminal(this);
    terminalList.append(newTerminal);
    
    // Connecter le signal de fermeture du terminal
    connect(newTerminal, &Terminal::terminalClosed, this, [this, newTerminal]() {
        int index = terminalList.indexOf(newTerminal);
        if (index >= 0) {
            closeTerminalTab(index);
        }
    });

    // Définir le répertoire de travail
    newTerminal->setWorkingDirectory(editorDirectoryOrWorkspace());

    // Ajouter dans les tabs avec emoji éclair
    int tabIndex = terminalTabs->addTab(newTerminal, QString("⚡ Terminal %1").arg(terminalList.size()));
    
    // Créer un bouton de fermeture personnalisé avec × visible
    QPushButton *closeBtn = new QPushButton("×", this);
    closeBtn->setFixedSize(16, 16);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    border: none;"
        "    border-radius: 3px;"
        "    color: #909090;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    padding: 0px;"
        "    margin: 0px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e06c75;"
        "    color: #ffffff;"
        "}"
    );
    connect(closeBtn, &QPushButton::clicked, this, [this, tabIndex]() {
        closeTerminalTab(tabIndex);
    });
    terminalTabs->tabBar()->setTabButton(tabIndex, QTabBar::RightSide, closeBtn);
    
    terminalTabs->setCurrentIndex(tabIndex);

    // Donner le focus au nouveau terminal
    newTerminal->focusTerminal();
}


void MainWindow::closeTerminalTab(int index)
{
    if (index < 0 || index >= terminalList.size()) return;

    Terminal *terminalToClose = terminalList.at(index);

    // Si c'est le dernier terminal, ne pas le fermer
    if (terminalList.size() == 1) {
        QMessageBox::information(this, tr("Cannot close terminal"),
                               tr("At least one terminal must remain open."));
        return;
    }

    // Fermer le terminal
    terminalList.removeAt(index);
    terminalTabs->removeTab(index);

    // Renommer les onglets restants et recréer leurs boutons de fermeture
    for (int i = 0; i < terminalList.size(); ++i) {
        terminalTabs->setTabText(i, QString("⚡ Terminal %1").arg(i + 1));

        // Supprimer l'ancien bouton de fermeture s'il existe
        QWidget *oldBtn = terminalTabs->tabBar()->tabButton(i, QTabBar::RightSide);
        if (oldBtn) {
            terminalTabs->tabBar()->setTabButton(i, QTabBar::RightSide, nullptr);
            oldBtn->deleteLater();
        }

        // Créer un nouveau bouton de fermeture
        QPushButton *closeBtn = new QPushButton("×", this);
        closeBtn->setFixedSize(16, 16);
        closeBtn->setCursor(Qt::PointingHandCursor);
        closeBtn->setStyleSheet(
            "QPushButton {"
            "    background-color: transparent;"
            "    border: none;"
            "    border-radius: 3px;"
            "    color: #909090;"
            "    font-size: 14px;"
            "    font-weight: bold;"
            "    padding: 0px;"
            "    margin: 0px;"
            "}"
            "QPushButton:hover {"
            "    background-color: #e06c75;"
            "    color: #ffffff;"
            "}"
        );
        connect(closeBtn, &QPushButton::clicked, this, [this, i]() {
            closeTerminalTab(i);
        });
        terminalTabs->tabBar()->setTabButton(i, QTabBar::RightSide, closeBtn);
    }

    // Supprimer le terminal
    terminalToClose->deleteLater();
}

void MainWindow::onTerminalTabChanged(int index)
{
    if (index >= 0 && index < terminalList.size()) {
        Terminal *currentTerminal = terminalList.at(index);
        // Mettre à jour le répertoire de travail si nécessaire
        currentTerminal->setWorkingDirectory(editorDirectoryOrWorkspace());
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    return QMainWindow::eventFilter(obj, event);
}
