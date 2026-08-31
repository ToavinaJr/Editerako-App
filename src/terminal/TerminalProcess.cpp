#include "terminal/TerminalProcess.h"

#include "core/Logging.h"

#include <QDir>
#include <QProcess>

TerminalProcess::TerminalProcess(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_workingDirectory(QDir::currentPath())
    , m_shell(defaultShell())
{
    connect(m_process, &QProcess::readyReadStandardOutput, this, &TerminalProcess::onReadyRead);
    connect(m_process, &QProcess::readyReadStandardError, this, &TerminalProcess::onReadyRead);
    connect(m_process, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus status) {
                if (m_stopping) {
                    return;
                }
                emit finished(exitCode, status == QProcess::CrashExit);
            });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (m_stopping) {
            return;
        }
        if (error == QProcess::FailedToStart) {
            emit failed(tr("Failed to start command"));
            return;
        }
        if (error == QProcess::Crashed) {
            return;
        }

        QString message;
        switch (error) {
        case QProcess::FailedToStart:
        case QProcess::Crashed:
            return;
        case QProcess::Timedout:
            message = tr("Process timed out");
            break;
        case QProcess::WriteError:
            message = tr("Write error");
            break;
        case QProcess::ReadError:
            message = tr("Read error");
            break;
        default:
            message = tr("Unknown process error");
            break;
        }
        emit failed(message);
    });
}

TerminalProcess::~TerminalProcess()
{
    stop();
}

QString TerminalProcess::defaultShell()
{
#ifdef Q_OS_WIN
    return QStringLiteral("cmd.exe");
#else
    const QString shell = qEnvironmentVariable("SHELL");
    return shell.isEmpty() ? QStringLiteral("/bin/bash") : shell;
#endif
}

void TerminalProcess::setWorkingDirectory(const QString &path)
{
    const QDir dir(path);
    if (dir.exists()) {
        m_workingDirectory = dir.absolutePath();
    }
}

bool TerminalProcess::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void TerminalProcess::startCommand(const QString &command)
{
    if (m_stopping || isRunning() || command.trimmed().isEmpty()) {
        return;
    }

    m_process->setWorkingDirectory(m_workingDirectory);

#ifdef Q_OS_WIN
    qCInfo(lcTerminal) << "Starting" << command << "in" << m_workingDirectory;
    m_process->start(QStringLiteral("cmd.exe"), {QStringLiteral("/c"), command});
#else
    qCInfo(lcTerminal) << "Starting" << command << "via" << m_shell << "in" << m_workingDirectory;
    m_process->start(m_shell, {QStringLiteral("-c"), command});
#endif
}

void TerminalProcess::stop()
{
    if (m_stopping) {
        return;
    }
    m_stopping = true;

    if (!m_process) {
        return;
    }

    blockSignals(true);
    m_process->disconnect(this);

    if (m_process->state() == QProcess::NotRunning) {
        return;
    }

    qCInfo(lcTerminal) << "Stopping process pid" << m_process->processId();
    m_process->kill();
    if (!m_process->waitForFinished(1000)) {
        m_process->close();
        m_process->waitForFinished(200);
    }
}

void TerminalProcess::onReadyRead()
{
    if (m_stopping || !m_process) {
        return;
    }
    const QByteArray output = m_process->readAllStandardOutput();
    const QByteArray error = m_process->readAllStandardError();
    if (!output.isEmpty()) {
        emit outputReady(QString::fromLocal8Bit(output), false);
    }
    if (!error.isEmpty()) {
        emit outputReady(QString::fromLocal8Bit(error), true);
    }
}
