#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "finddialog.h"
#include "gotolinedialog.h"
#include "chatwidget.h"
#include "core/CommandRegistry.h"
#include "editor/EditorDocument.h"
#include "editor/EditorManager.h"
#include "project/FileExplorer.h"
#include "project/Workspace.h"
#include "viewers/ViewerManager.h"
#include <QApplication>
#include <QAction>
#include <QKeySequence>
#include <QMenuBar>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include <QFileInfo>
#include <QTextStream>
#include <QHBoxLayout>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_editorManager(nullptr)
    , currentWorkingDirectory()
    , isFileTreeVisible(true)
{

    ui->setupUi(this);
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
    syncChatContext();
}

MainWindow::~MainWindow()
{
    shutdownAllTerminals();
    delete ui;
}


void MainWindow::connectActions()
{
    m_commands = new CommandRegistry(this);

    auto bind = [this](const QString &id, QAction *action, void (MainWindow::*slot)()) {
        m_commands->add(id, action);
        connect(action, &QAction::triggered, this, slot);
    };

    bind(QStringLiteral("file.new"), ui->actionFile, &MainWindow::newFile);
    bind(QStringLiteral("file.newFolder"), ui->actionNew_Document, &MainWindow::newFolder);
    bind(QStringLiteral("file.open"), ui->actionOpen_File, &MainWindow::openFile);
    bind(QStringLiteral("file.openFolder"), ui->actionOpen_Folder, &MainWindow::openFolder);
    bind(QStringLiteral("file.save"), ui->actionSave, &MainWindow::saveCurrentDocument);
    bind(QStringLiteral("file.saveAs"), ui->actionSave_As, &MainWindow::saveCurrentDocumentAs);
    bind(QStringLiteral("file.saveAll"), ui->actionSave_All, &MainWindow::saveAllDocuments);
    bind(QStringLiteral("file.close"), ui->actionClose, &MainWindow::closeCurrentTab);
    bind(QStringLiteral("file.closeOthers"), ui->actionClose_Others, &MainWindow::closeOtherTabs);
    bind(QStringLiteral("file.closeAll"), ui->actionClose_All, &MainWindow::closeAllTabs);
    bind(QStringLiteral("edit.find"), ui->actionFindReplace, &MainWindow::onActionFindReplace);
    bind(QStringLiteral("edit.gotoLine"), ui->actionGoToLine, &MainWindow::onActionGoToLine);

    ui->actionFile->setShortcut(QKeySequence::New);
    ui->actionOpen_File->setShortcut(QKeySequence::Open);
    ui->actionSave->setShortcut(QKeySequence::Save);
    ui->actionSave_As->setShortcut(QKeySequence::SaveAs);
    ui->actionClose->setShortcut(QKeySequence(QStringLiteral("Ctrl+W")));
    ui->actionFindReplace->setShortcut(QKeySequence::Find);
    ui->actionGoToLine->setShortcut(QKeySequence(QStringLiteral("Ctrl+G")));

    QAction *toggleTerminal = m_commands->create(
        QStringLiteral("view.terminal"),
        tr("Toggle Terminal"),
        QKeySequence(QStringLiteral("Ctrl+J")));
    connect(toggleTerminal, &QAction::triggered, this, &MainWindow::toggleTerminal);

    QMenu *viewMenu = menuBar()->addMenu(tr("View"));
    viewMenu->addAction(toggleTerminal);

    connect(ui->addFileButton, &QPushButton::clicked, this, &MainWindow::onAddFileClicked);
    connect(ui->newFolderButton, &QPushButton::clicked, this, &MainWindow::onNewFolderClicked);
    connect(ui->closeExplorerButton, &QPushButton::clicked, this, &MainWindow::onCloseExplorerClicked);
    connect(ui->checkBox, &QCheckBox::toggled, this, &MainWindow::onShowLinesToggled);

    updateCommandStates();
}

void MainWindow::updateCommandStates()
{
    if (!m_commands) {
        return;
    }

    const bool hasEditor = currentEditor() != nullptr;
    const int tabCount = m_editorManager ? m_editorManager->tabWidget()->count() : 0;

    m_commands->setEnabled(QStringLiteral("file.save"), hasEditor);
    m_commands->setEnabled(QStringLiteral("file.saveAs"), hasEditor);
    m_commands->setEnabled(QStringLiteral("file.saveAll"), hasEditor);
    m_commands->setEnabled(QStringLiteral("edit.find"), hasEditor);
    m_commands->setEnabled(QStringLiteral("edit.gotoLine"), hasEditor);
    m_commands->setEnabled(QStringLiteral("file.close"), tabCount > 0);
    m_commands->setEnabled(QStringLiteral("file.closeOthers"), tabCount > 1);
    m_commands->setEnabled(QStringLiteral("file.closeAll"), tabCount > 0);
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

    m_viewerManager = new ViewerManager(m_editorManager, this);

    connect(m_editorManager, &EditorManager::currentChanged, this, &MainWindow::updateWindowTitle);
    connect(m_editorManager, &EditorManager::currentChanged, this, &MainWindow::syncChatContext);
    connect(m_editorManager, &EditorManager::currentChanged, this, &MainWindow::updateCommandStates);
    connect(m_editorManager, &EditorManager::modificationChanged, this, &MainWindow::updateWindowTitle);
    connect(m_editorManager, &EditorManager::fileSaved, this, [this](const QString &path) {
        if (m_workspace && m_fileExplorer && m_workspace->containsPath(path)) {
            m_fileExplorer->reload();
        }
        if (statusBar()) {
            statusBar()->showMessage(tr("File saved successfully"), 2000);
        }
    });
}


void MainWindow::setupFileTree()
{
    m_workspace = new Workspace(this);
    m_fileExplorer = new FileExplorer(ui->fileTreeWidget, m_workspace, this);

    connect(m_fileExplorer, &FileExplorer::fileActivated, this, &MainWindow::openFileInEditor);
    connect(m_fileExplorer, &FileExplorer::fileSelected, this, [this](const QString &path) {
        if (statusBar()) {
            statusBar()->showMessage(tr("Selected: %1").arg(QFileInfo(path).fileName()), 2000);
        }
    });
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

    ok = dialog.exec();
    QString fileName = dialog.textValue();

    if (ok && !fileName.isEmpty()) {
        const QString targetDir = m_fileExplorer ? m_fileExplorer->selectedDirectory()
                                                 : currentWorkingDirectory;
        QString fullPath = QDir(targetDir).absoluteFilePath(fileName);
        QFile file(fullPath);

        if (file.open(QIODevice::WriteOnly)) {
            file.close();
            if (m_fileExplorer) {
                m_fileExplorer->reload();
                m_fileExplorer->revealPath(fullPath);
            }
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

    ok = dialog.exec();
    QString folderName = dialog.textValue();

    if (ok && !folderName.isEmpty()) {
        const QString targetDir = m_fileExplorer ? m_fileExplorer->selectedDirectory()
                                                 : currentWorkingDirectory;
        QString fullPath = QDir(targetDir).absoluteFilePath(folderName);
        QDir dir;
        if (dir.mkpath(fullPath)) {
            if (m_fileExplorer) {
                m_fileExplorer->reload();
                m_fileExplorer->revealPath(fullPath);
            }
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

void MainWindow::onShowLinesToggled(bool checked)
{
    if (currentEditor()) {
        currentEditor()->setLineNumbersVisible(checked);
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
        return;
    }
    if (result == ViewerManager::OpenResult::Unsupported) {
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
    QWidget *container = terminalContainer();
    if (!container) {
        return;
    }

    if (!isTerminalVisible) {
        if (terminalList.isEmpty()) {
            addNewTerminal();
        }
        isTerminalVisible = true;
        container->setVisible(true);

        const int index = terminalTabs ? terminalTabs->currentIndex() : -1;
        if (index >= 0 && index < terminalList.size()) {
            Terminal *currentTerminal = terminalList.at(index);
            currentTerminal->setWorkingDirectory(editorDirectoryOrWorkspace());
            currentTerminal->focusTerminal();
        }
    } else {
        isTerminalVisible = false;
        container->setVisible(false);
        if (currentEditor()) {
            currentEditor()->setFocus();
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

    shutdownAllTerminals();

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
    if (m_workspace) {
        m_workspace->setRootPath(path);
        currentWorkingDirectory = m_workspace->rootPath();
    } else {
        currentWorkingDirectory = path;
    }
    if (m_editorManager) {
        m_editorManager->setWorkingDirectory(currentWorkingDirectory);
    }
    if (m_fileExplorer) {
        m_fileExplorer->reload();
    }

    // Mettre à jour le répertoire de travail de tous les terminaux
    for (Terminal *terminal : terminalList) {
        terminal->setWorkingDirectory(currentWorkingDirectory);
    }

    if (chatWidget) {
        chatWidget->setProjectDirectory(currentWorkingDirectory);
    }

    // Update window title
    updateWindowTitle();
}

void MainWindow::syncChatContext()
{
    if (!chatWidget) {
        return;
    }

    CodeEditor *editor = m_editorManager ? m_editorManager->currentEditor() : nullptr;
    if (!editor) {
        chatWidget->setActiveFileContext({}, {});
        return;
    }

    QString path;
    if (EditorDocument *doc = EditorDocument::fromEditor(editor)) {
        path = doc->filePath();
        if (path.isEmpty()) {
            path = doc->displayName();
        }
    }
    chatWidget->setActiveFileContext(path, editor->toPlainText());
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
    terminalTabs->setObjectName(QStringLiteral("terminalTabs"));
    terminalTabs->setMovable(true);

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
    terminalTabs->addTab(firstTerminal, "⚡ Terminal 1");
    attachTerminalCloseButton(firstTerminal);

    // Créer le bouton + pour ajouter de nouveaux terminaux
    addTerminalButton = new QPushButton("+", this);
    addTerminalButton->setObjectName(QStringLiteral("addTerminalButton"));
    addTerminalButton->setFixedSize(28, 28);
    addTerminalButton->setToolTip(tr("Add new terminal"));
    addTerminalButton->setCursor(Qt::PointingHandCursor);

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
    attachTerminalCloseButton(newTerminal);

    if (QWidget *container = terminalContainer()) {
        container->setVisible(true);
        isTerminalVisible = true;
    }

    terminalTabs->setCurrentIndex(tabIndex);

    // Donner le focus au nouveau terminal
    newTerminal->focusTerminal();
}


void MainWindow::closeTerminalTab(int index)
{
    if (index < 0 || index >= terminalList.size()) {
        return;
    }

    Terminal *terminalToClose = terminalList.takeAt(index);
    terminalTabs->removeTab(index);

    terminalToClose->shutdown();
    terminalToClose->deleteLater();

    for (int i = 0; i < terminalList.size(); ++i) {
        terminalTabs->setTabText(i, QString("⚡ Terminal %1").arg(i + 1));
    }

    if (terminalList.isEmpty()) {
        if (QWidget *container = terminalContainer()) {
            container->setVisible(false);
        }
        isTerminalVisible = false;
        if (currentEditor()) {
            currentEditor()->setFocus();
        }
    }
}

void MainWindow::attachTerminalCloseButton(Terminal *terminal)
{
    if (!terminalTabs || !terminal) {
        return;
    }

    const int index = terminalTabs->indexOf(terminal);
    if (index < 0) {
        return;
    }

    QWidget *oldBtn = terminalTabs->tabBar()->tabButton(index, QTabBar::RightSide);
    if (oldBtn) {
        terminalTabs->tabBar()->setTabButton(index, QTabBar::RightSide, nullptr);
        oldBtn->deleteLater();
    }

    auto *closeBtn = new QPushButton(QStringLiteral("×"), terminalTabs->tabBar());
    closeBtn->setObjectName(QStringLiteral("terminalCloseButton"));
    closeBtn->setFixedSize(16, 16);
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, this, [this, terminal]() {
        const int idx = terminalList.indexOf(terminal);
        if (idx >= 0) {
            closeTerminalTab(idx);
        }
    });
    terminalTabs->tabBar()->setTabButton(index, QTabBar::RightSide, closeBtn);
}

void MainWindow::shutdownAllTerminals()
{
    for (Terminal *terminal : terminalList) {
        if (terminal) {
            terminal->shutdown();
        }
    }
}

QWidget *MainWindow::terminalContainer() const
{
    return terminalTabs ? terminalTabs->parentWidget() : nullptr;
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
