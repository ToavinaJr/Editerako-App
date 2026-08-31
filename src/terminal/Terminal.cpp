#include "terminal/Terminal.h"
#include "ui_Terminal.h"

#include "terminal/CommandDiscovery.h"
#include "terminal/TerminalInput.h"
#include "terminal/TerminalProcess.h"

#include <QDir>
#include <QScrollBar>
#include <QTextCursor>

Terminal::Terminal(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Terminal)
    , m_process(nullptr)
    , m_discovery(new CommandDiscovery(this))
    , m_completer(m_discovery, &m_history)
{
    ui->setupUi(this);
    setupTerminal();
    m_discovery->initialize();

    workingDirectory = QDir::currentPath();
    if (m_process) {
        m_process->setWorkingDirectory(workingDirectory);
    }

    displayPrompt();
}

Terminal::~Terminal()
{
    shutdown();
    delete ui;
}

void Terminal::setupTerminal()
{
    connect(ui->terminalOutput, &TerminalTextEdit::commandEntered,
            this, &Terminal::onCommandEntered);
    connect(ui->terminalOutput, &TerminalTextEdit::upPressed,
            this, &Terminal::onUpPressed);
    connect(ui->terminalOutput, &TerminalTextEdit::downPressed,
            this, &Terminal::onDownPressed);
    connect(ui->terminalOutput, &TerminalTextEdit::textChangedForAutoComplete,
            this, &Terminal::onTextChangedForAutoComplete);

    connect(ui->clearButton, &QPushButton::clicked,
            this, &Terminal::onClearClicked);
    connect(ui->closeButton, &QPushButton::clicked,
            this, &Terminal::onCloseClicked);

    m_process = new TerminalProcess(this);
    connect(m_process, &TerminalProcess::outputReady, this, &Terminal::onProcessOutput);
    connect(m_process, &TerminalProcess::finished, this, &Terminal::onProcessFinished);
    connect(m_process, &TerminalProcess::failed, this, &Terminal::onProcessFailed);
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
    const QString trimmedCommand = command.trimmed();

    if (trimmedCommand.isEmpty()) {
        displayPrompt();
        return;
    }

    m_history.add(trimmedCommand);

    if (trimmedCommand == QLatin1String("clear") || trimmedCommand == QLatin1String("cls")) {
        clearTerminal();
        return;
    }

    if (trimmedCommand.startsWith(QLatin1String("cd "))) {
        QString path = trimmedCommand.mid(3).trimmed();
        if (path.startsWith('"') && path.endsWith('"')) {
            path = path.mid(1, path.length() - 2);
        }

        QDir newDir(workingDirectory);
        if (path == QLatin1String("..")) {
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

    if (trimmedCommand == QLatin1String("pwd")) {
        appendOutput(workingDirectory + "\n", QColor(204, 204, 204));
        displayPrompt();
        return;
    }

    executeCommand(trimmedCommand);
}

void Terminal::shutdown()
{
    if (m_process) {
        m_process->stop();
    }
}

void Terminal::executeCommand(const QString &command)
{
    if (!m_process) {
        return;
    }
    if (m_process->isRunning()) {
        appendOutput("A command is already running. Please wait...\n",
                     QColor(229, 192, 123));
        displayPrompt();
        return;
    }

    m_process->setWorkingDirectory(workingDirectory);
    m_process->startCommand(command);
}

void Terminal::onProcessOutput(const QString &text, bool isError)
{
    appendOutput(text, isError ? QColor(224, 108, 117) : QColor(204, 204, 204));
}

void Terminal::onProcessFinished(int exitCode, bool crashed)
{
    if (crashed) {
        appendOutput("\nProcess crashed\n", QColor(224, 108, 117));
    } else if (exitCode != 0) {
        appendOutput(QString("\nProcess exited with code %1\n").arg(exitCode),
                     QColor(229, 192, 123));
    }

    displayPrompt();
}

void Terminal::onProcessFailed(const QString &message)
{
    appendOutput(message + "\n", QColor(224, 108, 117));
    if (!m_process || !m_process->isRunning()) {
        displayPrompt();
    }
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
    const CommandHistory::Navigation nav = m_history.navigate(direction);
    if (!nav.applied) {
        return;
    }
    ui->terminalOutput->clearCurrentCommand();
    if (!nav.clearLine) {
        ui->terminalOutput->insertPlainText(nav.command);
    }
}

void Terminal::setWorkingDirectory(const QString &path)
{
    QDir dir(path);
    if (dir.exists()) {
        workingDirectory = dir.absolutePath();
        if (m_process) {
            m_process->setWorkingDirectory(workingDirectory);
        }
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

void Terminal::updateAutoComplete()
{
    const QString currentCmd = ui->terminalOutput->getCurrentCommand().trimmed();
    if (currentCmd.isEmpty()) {
        ui->terminalOutput->hideAutoComplete();
        return;
    }

    const QStringList suggestions = m_completer.suggest(currentCmd, workingDirectory);
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

void Terminal::appendError(const QString &text)
{
    appendOutput(text, QColor(224, 108, 117));
}

void Terminal::appendSuccess(const QString &text)
{
    appendOutput(text, QColor(152, 195, 121));
}

void Terminal::appendInfo(const QString &text)
{
    appendOutput(text, QColor(97, 175, 239));
}
