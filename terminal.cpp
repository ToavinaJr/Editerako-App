#include "terminal.h"
#include "ui_terminal.h"
#include <QDir>
#include <QTextCursor>
#include <QScrollBar>
#include <QDateTime>
#include <QStandardPaths>
#include <QAbstractItemView>
#include <QApplication>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QFile>
#include <QTextStream>
#include <QSet>
#include <QDirIterator>
#include <QRegularExpression>

// ============================================================================
// AutoCompletePopup Implementation
// ============================================================================

AutoCompletePopup::AutoCompletePopup(QWidget *parent)
    : QListWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    
    // Ces attributs sont cruciaux pour ne pas voler le focus à l'éditeur
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
    
    // Le reste de votre style reste identique...
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionMode(QAbstractItemView::SingleSelection);

    // Modern dark theme styling
    setStyleSheet(
        "QListWidget {"
        "    background-color: #252526;"
        "    border: 1px solid #454545;"
        "    border-radius: 4px;"
        "    color: #cccccc;"
        "    font-family: 'Consolas', 'Monaco', monospace;"
        "    font-size: 12px;"
        "    padding: 4px;"
        "    outline: none;"
        "}"
        "QListWidget::item {"
        "    padding: 6px 12px;"
        "    border-radius: 3px;"
        "    margin: 1px 2px;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: #2a2d2e;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #094771;"
        "    color: #ffffff;"
        "}"
    );
    
    connect(this, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (item) {
            emit suggestionSelected(item->text());
            hide();
        }
    });
}

void AutoCompletePopup::showSuggestions(const QStringList &suggestions, const QPoint &position)
{
    clear();
    
    if (suggestions.isEmpty()) {
        hide();
        return;
    }
    
    addItems(suggestions);
    setCurrentRow(0);
    
    // Calculate optimal size
    int maxWidth = 300;
    int itemHeight = 30;
    int totalHeight = qMin(suggestions.count() * itemHeight + 10, 250);
    
    setFixedSize(maxWidth, totalHeight);
    move(position);
    show();
    raise();
}

QString AutoCompletePopup::currentSuggestion() const
{
    QListWidgetItem *item = currentItem();
    return item ? item->text() : QString();
}

void AutoCompletePopup::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit cancelled();
        hide();
        return;
    }
    
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Tab) {
        QString suggestion = currentSuggestion();
        if (!suggestion.isEmpty()) {
            emit suggestionSelected(suggestion);
            hide();
        }
        return;
    }
    
    QListWidget::keyPressEvent(event);
}

void AutoCompletePopup::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    hide();
}

// ============================================================================
// TerminalTextEdit Implementation
// ============================================================================

TerminalTextEdit::TerminalTextEdit(QWidget *parent)
    : QTextEdit(parent), promptPosition(0)
{
    setAcceptRichText(false);
    setUndoRedoEnabled(false);
    
    // Create autocomplete popup
    autoCompletePopup = new AutoCompletePopup(this);
    connect(autoCompletePopup, &AutoCompletePopup::suggestionSelected,
            this, &TerminalTextEdit::acceptSuggestion);
    connect(autoCompletePopup, &AutoCompletePopup::cancelled,
            this, &TerminalTextEdit::hideAutoComplete);
}

void TerminalTextEdit::setPrompt(const QString &prompt)
{
    currentPrompt = prompt;
    promptPosition = textCursor().position();
}

QString TerminalTextEdit::getCurrentCommand() const
{
    QString fullText = toPlainText();
    if (fullText.length() <= promptPosition) {
        return QString();
    }
    return fullText.mid(promptPosition);
}

void TerminalTextEdit::clearCurrentCommand()
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(promptPosition);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
}

void TerminalTextEdit::keyPressEvent(QKeyEvent *event)
{
    // Si la popup est visible, on intercepte SEULEMENT la navigation
    if (autoCompletePopup->isVisible()) {
        // Navigation HAUT
        if (event->key() == Qt::Key_Up) {
            int currentRow = autoCompletePopup->currentRow();
            if (currentRow > 0) {
                autoCompletePopup->setCurrentRow(currentRow - 1);
            }
            return; // On consomme l'événement
        }
        
        // Navigation BAS
        if (event->key() == Qt::Key_Down) {
            int currentRow = autoCompletePopup->currentRow();
            if (currentRow < autoCompletePopup->count() - 1) {
                autoCompletePopup->setCurrentRow(currentRow + 1);
            }
            return; // On consomme l'événement
        }
        
        // Validation (Tab ou Entrée SEULEMENT si une sélection est active)
        if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            QString suggestion = autoCompletePopup->currentSuggestion();
            if (!suggestion.isEmpty()) {
                acceptSuggestion(suggestion);
                return; // On consomme l'événement
            }
            // Si pas de suggestion valide, on laisse passer (pour le saut de ligne normal)
        }
        
        // Annulation
        if (event->key() == Qt::Key_Escape) {
            hideAutoComplete();
            return;
        }
        
    }
    
    // Gestion de l'entrée commande standard
    bool isEnter = (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter);

    if (isEnter && !autoCompletePopup->isVisible()) {
        hideAutoComplete();
        QString command = getCurrentCommand();
        moveCursor(QTextCursor::End);
        append("");
        emit commandEntered(command);
        return;
    }

    // Empêcher la modification avant le prompt
    QTextCursor cursor = textCursor();
    if (cursor.position() < promptPosition) {
        if (event->key() == Qt::Key_Backspace ||
            event->key() == Qt::Key_Delete ||
            event->key() == Qt::Key_Left) {
            return; // Ignore les touches qui modifieraient avant le prompt
        }
        moveCursor(QTextCursor::End); // Déplace le curseur à la fin si on tape avant le prompt
    }

    // Empêcher le backspace à la position du prompt
    if (event->key() == Qt::Key_Backspace && cursor.position() <= promptPosition) {
        return;
    }

    // Appel au parent pour écrire le texte dans le terminal
    QTextEdit::keyPressEvent(event);
    
    // Mise à jour de l'autocomplétion APRÈS que le caractère soit inséré
    if (!event->text().isEmpty() && !isEnter && event->key() != Qt::Key_Escape && event->key() != Qt::Key_Backspace) {
        emit textChangedForAutoComplete();
    }
    // Gestion spéciale du Backspace pour rafraichir aussi la liste
    else if (event->key() == Qt::Key_Backspace) {
        emit textChangedForAutoComplete();
    }
}

void TerminalTextEdit::mousePressEvent(QMouseEvent *event)
{
    QTextEdit::mousePressEvent(event);
    ensureCursorInEditableArea();
}

void TerminalTextEdit::mouseDoubleClickEvent(QMouseEvent *event)
{
    QTextEdit::mouseDoubleClickEvent(event);
    ensureCursorInEditableArea();
}

void TerminalTextEdit::ensureCursorInEditableArea()
{
    QTextCursor cursor = textCursor();
    if (cursor.position() < promptPosition) {
        cursor.setPosition(promptPosition);
        setTextCursor(cursor);
    }
}

int TerminalTextEdit::getPromptPosition() const
{
    return promptPosition;
}

void TerminalTextEdit::showAutoComplete(const QStringList &suggestions)
{
    if (suggestions.isEmpty()) {
        hideAutoComplete();
        return;
    }
    
    // Calculate popup position below the cursor
    QTextCursor cursor = textCursor();
    QRect cursorRectangle = cursorRect(cursor);
    QPoint globalPos = mapToGlobal(cursorRectangle.bottomLeft());
    
    autoCompletePopup->showSuggestions(suggestions, globalPos);
}

void TerminalTextEdit::hideAutoComplete()
{
    autoCompletePopup->hide();
}

void TerminalTextEdit::acceptSuggestion(const QString &suggestion)
{
    // Get current command and find the word being completed
    QString cmd = getCurrentCommand();
    QStringList parts = cmd.split(' ', Qt::SkipEmptyParts);
    
    if (parts.isEmpty()) {
        insertPlainText(suggestion + " ");
    } else {
        // Replace the last partial word with the suggestion
        QString lastWord = parts.last();
        for (int i = 0; i < lastWord.length(); ++i) {
            QTextCursor cursor = textCursor();
            cursor.deletePreviousChar();
            setTextCursor(cursor);
        }
        insertPlainText(suggestion + " ");
    }
    
    hideAutoComplete();
}

// ============================================================================
// Terminal Implementation
// ============================================================================

Terminal::Terminal(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Terminal)
    , process(nullptr)
    , historyIndex(-1)
    , isProcessRunning(false)
{
    ui->setupUi(this);
    setupTerminal();
    initializeShell();
    initializeCommandDatabase();

    // Set default working directory
    workingDirectory = QDir::currentPath();

    displayPrompt();
    isDragging = false;

    // Activer le drag sur la toolbar
    ui->terminalToolbar->setMouseTracking(true);
    ui->terminalToolbar->installEventFilter(this);

}

Terminal::~Terminal()
{
    if (process && process->state() == QProcess::Running) {
        process->kill();
        process->waitForFinished();
    }
    delete ui;
}

void Terminal::setupTerminal()
{
    // Connect terminal text edit signals
    connect(ui->terminalOutput, &TerminalTextEdit::commandEntered,
            this, &Terminal::onCommandEntered);
    connect(ui->terminalOutput, &TerminalTextEdit::upPressed,
            this, &Terminal::onUpPressed);
    connect(ui->terminalOutput, &TerminalTextEdit::downPressed,
            this, &Terminal::onDownPressed);
    connect(ui->terminalOutput, &TerminalTextEdit::textChangedForAutoComplete,
            this, &Terminal::onTextChangedForAutoComplete);

    // Connect toolbar buttons
    connect(ui->clearButton, &QPushButton::clicked,
            this, &Terminal::onClearClicked);
    connect(ui->closeButton, &QPushButton::clicked,
            this, &Terminal::onCloseClicked);

    // Setup process
    process = new QProcess(this);
    connect(process, &QProcess::readyReadStandardOutput,
            this, &Terminal::onProcessReadyRead);
    connect(process, &QProcess::readyReadStandardError,
            this, &Terminal::onProcessReadyRead);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Terminal::onProcessFinished);
    connect(process, &QProcess::errorOccurred,
            this, &Terminal::onProcessError);
}

void Terminal::initializeShell()
{
#ifdef Q_OS_WIN
    currentShell = "cmd.exe";
#else
    currentShell = getSystemShell();
#endif
}

QString Terminal::getSystemShell()
{
    QString shell = qEnvironmentVariable("SHELL");
    if (shell.isEmpty()) {
        shell = "/bin/bash";
    }
    return shell;
}

void Terminal::displayPrompt()
{
    QString prompt;
    QDir dir(workingDirectory);

#ifdef Q_OS_WIN
    prompt = QString("%1> ").arg(dir.absolutePath());
#else
    prompt = QString("%1$ ").arg(dir.dirName());
#endif

    QTextCursor cursor = ui->terminalOutput->textCursor();
    cursor.movePosition(QTextCursor::End);

    QTextCharFormat format;
    format.setForeground(QColor(152, 195, 121)); 
    cursor.setCharFormat(format);
    cursor.insertText(prompt);

    ui->terminalOutput->setTextCursor(cursor);
    ui->terminalOutput->setPrompt(prompt);
}

void Terminal::appendOutput(const QString &text, const QColor &color)
{
    QTextCursor cursor = ui->terminalOutput->textCursor();
    cursor.movePosition(QTextCursor::End);

    QTextCharFormat format;
    format.setForeground(color);
    cursor.setCharFormat(format);
    cursor.insertText(text);

    ui->terminalOutput->setTextCursor(cursor);
    ui->terminalOutput->verticalScrollBar()->setValue(
        ui->terminalOutput->verticalScrollBar()->maximum());
}

void Terminal::onCommandEntered(const QString &command)
{
    QString trimmedCommand = command.trimmed();

    if (trimmedCommand.isEmpty()) {
        displayPrompt();
        return;
    }

    // Add to history
    if (!trimmedCommand.isEmpty() &&
        (commandHistory.isEmpty() || commandHistory.last() != trimmedCommand)) {
        commandHistory.append(trimmedCommand);
    }
    historyIndex = commandHistory.size();

    // Process internal commands
    if (trimmedCommand == "clear" || trimmedCommand == "cls") {
        clearTerminal();
        return;
    }

    if (trimmedCommand.startsWith("cd ")) {
        QString path = trimmedCommand.mid(3).trimmed();
        if (path.startsWith("\"") && path.endsWith("\"")) {
            path = path.mid(1, path.length() - 2);
        }

        QDir newDir(workingDirectory);
        if (path == "..") {
            newDir.cdUp();
        } else if (QDir::isAbsolutePath(path)) {
            newDir = QDir(path);
        } else {
            newDir.cd(path);
        }

        if (newDir.exists()) {
            workingDirectory = newDir.absolutePath();
            displayPrompt();
        } else {
            appendOutput("Directory not found: " + path + "\n", QColor(224, 108, 117));
            displayPrompt();
        }
        return;
    }

    if (trimmedCommand == "pwd") {
        appendOutput(workingDirectory + "\n", QColor(204, 204, 204));
        displayPrompt();
        return;
    }

    // Execute external command
    executeCommand(trimmedCommand);
}

void Terminal::executeCommand(const QString &command)
{
    if (isProcessRunning) {
        appendOutput("A command is already running. Please wait...\n",
                     QColor(229, 192, 123));
        displayPrompt();
        return;
    }

    isProcessRunning = true;
    process->setWorkingDirectory(workingDirectory);

#ifdef Q_OS_WIN
    process->start("cmd.exe", QStringList() << "/c" << command);
#else
    process->start(currentShell, QStringList() << "-c" << command);
#endif

    if (!process->waitForStarted(3000)) {
        appendError("Failed to start command");
        isProcessRunning = false;
        displayPrompt();
    }
}

void Terminal::onProcessReadyRead()
{
    QByteArray output = process->readAllStandardOutput();
    QByteArray error = process->readAllStandardError();

    if (!output.isEmpty()) {
        appendOutput(QString::fromLocal8Bit(output), QColor(204, 204, 204)); // Default color for stdout
    }

    if (!error.isEmpty()) {
        appendOutput(QString::fromLocal8Bit(error), QColor(224, 108, 117)); // Red for stderr
    }
}

void Terminal::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    isProcessRunning = false;

    if (exitStatus == QProcess::CrashExit) {
        appendOutput("\nProcess crashed\n", QColor(224, 108, 117));
    } else if (exitCode != 0) {
        appendOutput(QString("\nProcess exited with code %1\n").arg(exitCode),
                     QColor(229, 192, 123));
    }

    displayPrompt();
}

void Terminal::onProcessError(QProcess::ProcessError error)
{
    isProcessRunning = false;

    QString errorMsg;
    switch (error) {
    case QProcess::FailedToStart:
        errorMsg = "Failed to start process";
        break;
    case QProcess::Crashed:
        errorMsg = "Process crashed";
        break;
    case QProcess::Timedout:
        errorMsg = "Process timed out";
        break;
    case QProcess::WriteError:
        errorMsg = "Write error";
        break;
    case QProcess::ReadError:
        errorMsg = "Read error";
        break;
    default:
        errorMsg = "Unknown error";
    }

    appendOutput(errorMsg + "\n", QColor(224, 108, 117));
    displayPrompt();
}

void Terminal::onUpPressed()
{
    navigateHistory(-1);
}

void Terminal::onDownPressed()
{
    navigateHistory(1);
}

void Terminal::navigateHistory(int direction)
{
    if (commandHistory.isEmpty()) {
        return;
    }

    historyIndex += direction;

    if (historyIndex < 0) {
        historyIndex = 0;
    } else if (historyIndex >= commandHistory.size()) {
        historyIndex = commandHistory.size();
        ui->terminalOutput->clearCurrentCommand();
        return;
    }

    ui->terminalOutput->clearCurrentCommand();
    ui->terminalOutput->insertPlainText(commandHistory[historyIndex]);
}

void Terminal::setWorkingDirectory(const QString &path)
{
    QDir dir(path);
    if (dir.exists()) {
        workingDirectory = dir.absolutePath();
    }
}

QString Terminal::getWorkingDirectory() const
{
    return workingDirectory;
}

void Terminal::clearTerminal()
{
    ui->terminalOutput->clear();
    displayPrompt();
}

void Terminal::onClearClicked()
{
    clearTerminal();
}

void Terminal::onCloseClicked()
{
    emit terminalClosed();
}

void Terminal::focusTerminal()
{
    ui->terminalOutput->setFocus();
    ui->terminalOutput->moveCursor(QTextCursor::End);
}

void Terminal::initializeCommandDatabase()
{
#ifdef Q_OS_WIN
    // Common Windows commands
    commonCommands << "cd" << "dir" << "cls" << "copy" << "move" << "del" << "mkdir"
                   << "rmdir" << "type" << "echo" << "set" << "path" << "exit"
                   << "help" << "start" << "tasklist" << "taskkill" << "ipconfig"
                   << "ping" << "netstat" << "systeminfo" << "chkdsk" << "diskpart"
                   << "format" << "attrib" << "xcopy" << "robocopy" << "findstr"
                   << "tree" << "fc" << "more" << "sort" << "find";
    
    // Git commands (common on Windows too)
    commonCommands << "git" << "npm" << "node" << "python" << "pip" << "cargo"
                   << "rustc" << "cmake" << "make" << "gcc" << "g++" << "clang";
    
    // Command arguments
    commandArguments["cd"] = QStringList() << ".." << "." << "/d";
    commandArguments["dir"] = QStringList() << "/a" << "/b" << "/s" << "/p" << "/w";
    commandArguments["copy"] = QStringList() << "/y" << "/v" << "/z";
    commandArguments["del"] = QStringList() << "/p" << "/f" << "/s" << "/q";
    commandArguments["git"] = QStringList() << "clone" << "pull" << "push" << "commit"
                                            << "add" << "status" << "log" << "branch"
                                            << "checkout" << "merge" << "rebase" << "init";
    commandArguments["npm"] = QStringList() << "install" << "run" << "start" << "build"
                                            << "test" << "init" << "update" << "uninstall";
    commandArguments["pip"] = QStringList() << "install" << "uninstall" << "list" << "show"
                                            << "freeze" << "search" << "upgrade";
#else
    // Common Unix/Linux commands
    commonCommands << "ls" << "cd" << "pwd" << "mkdir" << "rmdir" << "rm" << "cp"
                   << "mv" << "touch" << "cat" << "grep" << "find" << "chmod"
                   << "chown" << "ps" << "kill" << "top" << "df" << "du" << "tar"
                   << "gzip" << "gunzip" << "wget" << "curl" << "ssh" << "scp"
                   << "git" << "npm" << "node" << "python" << "pip" << "make"
                   << "gcc" << "g++" << "sudo" << "apt" << "yum" << "systemctl";
    
    commandArguments["ls"] = QStringList() << "-l" << "-a" << "-h" << "-R" << "-t";
    commandArguments["rm"] = QStringList() << "-r" << "-f" << "-i" << "-v";
    commandArguments["cp"] = QStringList() << "-r" << "-i" << "-v" << "-p";
    commandArguments["chmod"] = QStringList() << "755" << "644" << "777" << "-R";
    commandArguments["git"] = QStringList() << "clone" << "pull" << "push" << "commit"
                                            << "add" << "status" << "log" << "branch"
                                            << "checkout" << "merge" << "rebase" << "init";
#endif
    // Load cached commands (if any) for instant suggestions
    loadCommandCache();

    // Kick off an asynchronous system scan to find all executables in PATH
    scanSystemCommandsAsync();
}

QStringList Terminal::getCommandSuggestions(const QString &partial)
{
    QStringList suggestions;
    QString lowerPartial = partial.toLower();
    
    // Search in common commands
    for (const QString &cmd : commonCommands) {
        if (cmd.startsWith(lowerPartial, Qt::CaseInsensitive)) {
            suggestions << cmd;
        }
    }
    
    // Search in command history
    for (const QString &histCmd : commandHistory) {
        QString firstWord = histCmd.split(' ').first();
        if (firstWord.startsWith(lowerPartial, Qt::CaseInsensitive) && !suggestions.contains(firstWord)) {
            suggestions << firstWord;
        }
    }
    
    suggestions.sort(Qt::CaseInsensitive);
    return suggestions;
}

// Load cached commands from a simple cache file to provide instant suggestions
void Terminal::loadCommandCache()
{
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (cacheDir.isEmpty()) return;
    QDir().mkpath(cacheDir);
    QString cacheFile = cacheDir + QDir::separator() + "commands_cache.txt";

    QFile f(cacheFile);
    if (!f.exists()) return;
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&f);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty() && !commonCommands.contains(line)) {
            commonCommands << line;
        }
    }
    f.close();
    commonCommands.removeDuplicates();
    commonCommands.sort(Qt::CaseInsensitive);
}

// Save current command list to cache file
void Terminal::saveCommandCache()
{
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (cacheDir.isEmpty()) return;
    QDir().mkpath(cacheDir);
    QString cacheFile = cacheDir + QDir::separator() + "commands_cache.txt";

    QFile f(cacheFile);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&f);
    for (const QString &cmd : commonCommands) {
        out << cmd << "\n";
    }
    f.close();
}

// Asynchronously scan system PATH for executables and merge into commonCommands
void Terminal::scanSystemCommandsAsync()
{
    auto future = QtConcurrent::run([this]() -> QStringList {
        QStringList result;
        QString pathEnv = qEnvironmentVariable("PATH");
        QStringList paths = pathEnv.split(QDir::listSeparator(), Qt::SkipEmptyParts);
        // Add common system dirs on macOS/Linux
#ifndef Q_OS_WIN
        paths.append("/usr/local/bin");
        paths.append("/opt/homebrew/bin");
#endif
        QSet<QString> seen;
        for (const QString &p : paths) {
            QDir dir(p);
            if (!dir.exists()) continue;
            QDirIterator it(dir.absolutePath(), QDir::Files | QDir::NoSymLinks, QDirIterator::NoIteratorFlags);
            while (it.hasNext()) {
                it.next();
                QFileInfo fi = it.fileInfo();
                // Windows: consider common executable extensions
#ifdef Q_OS_WIN
                QString ext = fi.suffix().toLower();
                if (ext == "exe" || ext == "bat" || ext == "cmd" || ext == "com") {
                    seen.insert(fi.baseName());
                }
#else
                if (fi.isExecutable()) {
                    seen.insert(fi.fileName());
                }
#endif
            }
        }
        // Convert QSet to QStringList
        for (const QString &s : seen) {
            result << s;
        }
        result.sort(Qt::CaseInsensitive);
        return result;
    });

    QFutureWatcher<QStringList> *watcher = new QFutureWatcher<QStringList>(this);
    connect(watcher, &QFutureWatcher<QStringList>::finished, this, [this, watcher]() {
        QStringList found = watcher->result();
        bool changed = false;
        for (const QString &c : found) {
            if (!commonCommands.contains(c)) {
                commonCommands << c;
                changed = true;
            }
        }
        if (changed) {
            commonCommands.removeDuplicates();
            commonCommands.sort(Qt::CaseInsensitive);
            saveCommandCache();
        }
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

// Load cached arguments for a specific command
QStringList Terminal::loadCachedArguments(const QString &command)
{
    QStringList result;
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (cacheDir.isEmpty()) return result;
    QDir().mkpath(cacheDir);
    QString safeName = command;
    // sanitize filename
    safeName.remove(QRegularExpression("[^A-Za-z0-9_.-]"));
    if (safeName.isEmpty()) return result;
    QString cacheFile = cacheDir + QDir::separator() + QString("args_%1.txt").arg(safeName);

    QFile f(cacheFile);
    if (!f.exists()) return result;
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return result;
    QTextStream in(&f);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) result << line;
    }
    f.close();
    result.removeDuplicates();
    result.sort(Qt::CaseInsensitive);
    return result;
}

// Save cached arguments for a specific command
void Terminal::saveCachedArguments(const QString &command)
{
    if (!commandArguments.contains(command)) return;
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (cacheDir.isEmpty()) return;
    QDir().mkpath(cacheDir);
    QString safeName = command;
    safeName.remove(QRegularExpression("[^A-Za-z0-9_.-]"));
    if (safeName.isEmpty()) return;
    QString cacheFile = cacheDir + QDir::separator() + QString("args_%1.txt").arg(safeName);

    QFile f(cacheFile);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&f);
    const QStringList &args = commandArguments[command];
    for (const QString &a : args) {
        out << a << "\n";
    }
    f.close();
}

// Asynchronously discover possible arguments for a given command by parsing its help output
void Terminal::scanCommandArgumentsAsync(const QString &command)
{
    if (command.trimmed().isEmpty()) return;
    // run in background thread
    auto future = QtConcurrent::run([command]() -> QStringList {
        QStringList result;
        auto tryRun = [&](const QString &prog, const QStringList &args) -> QString {
            QProcess p;
            p.start(prog, args);
            if (!p.waitForStarted(1000)) return QString();
            p.waitForFinished(2000);
            QByteArray out = p.readAllStandardOutput() + p.readAllStandardError();
            QString s = QString::fromLocal8Bit(out);
            return s.trimmed();
        };

        QString helpOut;
        // common help forms
        helpOut = tryRun(command, QStringList() << "--help");
        if (helpOut.isEmpty()) helpOut = tryRun(command, QStringList() << "-h");
        if (helpOut.isEmpty()) helpOut = tryRun(command, QStringList() << "help");
        if (helpOut.isEmpty()) {
#ifndef Q_OS_WIN
            // try man
            helpOut = tryRun(QStringLiteral("man"), QStringList() << command);
#endif
        }
        if (helpOut.isEmpty()) {
            // try "command help" form
            helpOut = tryRun(command, QStringList() << "help");
        }

        if (helpOut.isEmpty()) return result;

        // limit to first 500 lines to avoid huge outputs
        QStringList lines = helpOut.split('\n');
        if (lines.size() > 500) lines = lines.mid(0, 500);
        QString snippet = lines.join('\n');

        // regex to extract flags: -a, --long, /option and options with =VALUE
        QRegularExpression re(R"((?:^|[\s,;()\[\]])(-{1,2}[A-Za-z0-9][A-Za-z0-9._-]*(?:[= ][A-Za-z0-9_<>\\[\]-]+)?|/[A-Za-z0-9._-]+))");
        QSet<QString> seen;
        QRegularExpressionMatchIterator it = re.globalMatch(snippet);
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            QString token = m.captured(1).trimmed();
            if (!token.isEmpty()) seen.insert(token);
        }

        // Additional common patterns: short combined flags like -abc -> -a -b -c
        QStringList extras;
        for (const QString &t : seen) {
            if (t.startsWith('-') && t.length() > 2 && !t.startsWith("--") && !t.contains('=')) {
                // split combined like -abc into -a, -b, -c
                for (int i = 1; i < t.length(); ++i) {
                    QString s = QString("-%1").arg(t.at(i));
                    extras << s;
                }
            }
        }
        for (const QString &e : extras) seen.insert(e);

        for (const QString &s : seen) result << s;
        result.removeDuplicates();
        result.sort(Qt::CaseInsensitive);
        return result;
    });

    QFutureWatcher<QStringList> *watcher = new QFutureWatcher<QStringList>(this);
    connect(watcher, &QFutureWatcher<QStringList>::finished, this, [this, watcher, command]() {
        QStringList found = watcher->result();
        if (!found.isEmpty()) {
            commandArguments[command] = found;
            saveCachedArguments(command);
        }
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

QStringList Terminal::getArgumentSuggestions(const QString &command, const QString &partial)
{
    QStringList suggestions;
    
    if (commandArguments.contains(command)) {
        const QStringList &args = commandArguments[command];
        for (const QString &arg : args) {
            if (arg.startsWith(partial, Qt::CaseInsensitive)) {
                suggestions << arg;
            }
        }
    }
    
    return suggestions;
}

QStringList Terminal::getPathSuggestions(const QString &partial)
{
    QStringList suggestions;
    
    QString basePath = partial;
    QString searchPattern = "*";
    
    // Extract directory and filename pattern
    int lastSlash = partial.lastIndexOf('/');
    if (lastSlash == -1) {
        lastSlash = partial.lastIndexOf('\\');
    }
    
    if (lastSlash != -1) {
        basePath = partial.left(lastSlash + 1);
        searchPattern = partial.mid(lastSlash + 1) + "*";
    } else {
        basePath = "";
        searchPattern = partial + "*";
    }
    
    // Determine search directory
    QDir searchDir;
    if (basePath.isEmpty()) {
        searchDir = QDir(workingDirectory);
    } else if (QDir::isAbsolutePath(basePath)) {
        searchDir = QDir(basePath);
    } else {
        searchDir = QDir(workingDirectory + "/" + basePath);
    }
    
    if (!searchDir.exists()) {
        return suggestions;
    }
    
    // Get matching entries
    QStringList entries = searchDir.entryList(
        QStringList() << searchPattern,
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
        QDir::Name
    );
    
    for (const QString &entry : entries) {
        QString fullPath = basePath + entry;
        QFileInfo info(searchDir.absoluteFilePath(entry));
        if (info.isDir()) {
            fullPath += "/";
        }
        suggestions << fullPath;
    }
    
    return suggestions;
}

void Terminal::updateAutoComplete()
{
    QString currentCmd = ui->terminalOutput->getCurrentCommand().trimmed();
    
    if (currentCmd.isEmpty()) {
        ui->terminalOutput->hideAutoComplete();
        return;
    }
    
    QStringList parts = currentCmd.split(' ', Qt::SkipEmptyParts);
    QStringList suggestions;
    
    if (parts.isEmpty()) {
        ui->terminalOutput->hideAutoComplete();
        return;
    }
    
    // First word - suggest commands
    if (parts.size() == 1) {
        suggestions = getCommandSuggestions(parts[0]);
    }
    // Subsequent words - suggest arguments or paths
    else {
        QString command = parts[0];
        QString lastPart = parts.last();

        // Ensure we have cached arguments for this command; if not, try loading cache and schedule async scan
        if (!commandArguments.contains(command)) {
            QStringList cached = loadCachedArguments(command);
            if (!cached.isEmpty()) {
                commandArguments[command] = cached;
            } else {
                // Launch async scan to populate arguments for this command
                scanCommandArgumentsAsync(command);
            }
        }

        // Try argument suggestions first
        suggestions = getArgumentSuggestions(command, lastPart);
        
        // If no argument matches, try path suggestions
        if (suggestions.isEmpty()) {
            suggestions = getPathSuggestions(lastPart);
        }
    }
    
    // Limit suggestions to avoid overwhelming the user
    if (suggestions.size() > 15) {
        suggestions = suggestions.mid(0, 15);
    }
    
    if (!suggestions.isEmpty()) {
        ui->terminalOutput->showAutoComplete(suggestions);
    } else {
        ui->terminalOutput->hideAutoComplete();
    }
}

void Terminal::onTextChangedForAutoComplete()
{
    updateAutoComplete();
}

void Terminal::onSuggestionSelected(const QString &suggestion)
{
    // Handled by TerminalTextEdit::acceptSuggestion
}

void Terminal::appendError(const QString &text)
{
    appendOutput(text, QColor(224, 108, 117)); // Red color for errors
}

void Terminal::appendSuccess(const QString &text)
{
    appendOutput(text, QColor(152, 195, 121)); // Green color for success
}

void Terminal::appendInfo(const QString &text)
{
    appendOutput(text, QColor(97, 175, 239)); // Blue color for info
}

