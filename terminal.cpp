#include "terminal.h"
#include "ui_terminal.h"
#include <QDir>
#include <QTextCursor>
#include <QScrollBar>
#include <QDateTime>
#include <QStandardPaths>

// ============================================================================
// TerminalTextEdit Implementation
// ============================================================================

TerminalTextEdit::TerminalTextEdit(QWidget *parent)
    : QTextEdit(parent), promptPosition(0)
{
    setAcceptRichText(false);
    setUndoRedoEnabled(false);
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
    // Handle special keys
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QString command = getCurrentCommand();
        moveCursor(QTextCursor::End);
        append("");
        emit commandEntered(command);
        return;
    }

    if (event->key() == Qt::Key_Up) {
        emit upPressed();
        return;
    }

    if (event->key() == Qt::Key_Down) {
        emit downPressed();
        return;
    }

    if (event->key() == Qt::Key_Tab) {
        emit tabPressed();
        return;
    }

    // Prevent editing before prompt
    QTextCursor cursor = textCursor();
    if (cursor.position() < promptPosition) {
        if (event->key() == Qt::Key_Backspace ||
            event->key() == Qt::Key_Delete ||
            event->key() == Qt::Key_Left) {
            return;
        }
        moveCursor(QTextCursor::End);
    }

    // Handle backspace at prompt position
    if (event->key() == Qt::Key_Backspace && cursor.position() <= promptPosition) {
        return;
    }

    // Handle Ctrl+C to cancel current command
    if (event->matches(QKeySequence::Copy) && !textCursor().hasSelection()) {
        clearCurrentCommand();
        append("");
        emit commandEntered("");
        return;
    }

    QTextEdit::keyPressEvent(event);
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
    format.setForeground(QColor(152, 195, 121)); // Green color for prompt
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
        appendOutput("Failed to start command\n", QColor(224, 108, 117));
        isProcessRunning = false;
        displayPrompt();
    }
}

void Terminal::onProcessReadyRead()
{
    QByteArray output = process->readAllStandardOutput();
    QByteArray error = process->readAllStandardError();

    if (!output.isEmpty()) {
        appendOutput(QString::fromLocal8Bit(output), QColor(204, 204, 204));
    }

    if (!error.isEmpty()) {
        appendOutput(QString::fromLocal8Bit(error), QColor(224, 108, 117));
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

