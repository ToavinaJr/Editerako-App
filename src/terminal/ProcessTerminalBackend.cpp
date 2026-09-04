#include "terminal/ProcessTerminalBackend.h"

#include "core/Logging.h"

#include <QDir>
#include <QProcess>
#include <QProcessEnvironment>

ProcessTerminalBackend::ProcessTerminalBackend(QObject *parent)
    : ITerminalBackend(parent)
    , m_process(new QProcess(this))
{
    connect(m_process, &QProcess::readyReadStandardOutput, this,
            &ProcessTerminalBackend::onReadyRead);
    connect(m_process, &QProcess::readyReadStandardError, this,
            &ProcessTerminalBackend::onReadyRead);
    connect(m_process, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus status) {
                if (m_stopping) {
                    return;
                }
                emit finished(exitCode, status == QProcess::CrashExit);
            });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (m_stopping || error == QProcess::Crashed) {
            return;
        }
        if (error == QProcess::FailedToStart) {
            emit failed(tr("Failed to start command"));
        }
    });
}

ProcessTerminalBackend::~ProcessTerminalBackend()
{
    stop();
}

void ProcessTerminalBackend::start(const QString &program, const QStringList &arguments,
                                   const QString &workingDirectory, int columns, int rows)
{
    if (isRunning() || program.isEmpty()) {
        return;
    }
    m_stopping = false;
    resize(columns, rows);
    m_process->setWorkingDirectory(workingDirectory.isEmpty() ? QDir::currentPath()
                                                              : workingDirectory);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("COLUMNS"), QString::number(qMax(20, m_columns)));
    env.insert(QStringLiteral("LINES"), QString::number(qMax(5, m_rows)));
    env.insert(QStringLiteral("TERM"), QStringLiteral("xterm-256color"));
    m_process->setProcessEnvironment(env);
    qCInfo(lcTerminal) << "Process backend" << program << arguments << "in"
                       << m_process->workingDirectory();
    m_process->start(program, arguments);
}

void ProcessTerminalBackend::write(const QByteArray &data)
{
    if (isRunning() && !data.isEmpty()) {
        m_process->write(data);
    }
}

void ProcessTerminalBackend::resize(int columns, int rows)
{
    if (columns > 0) {
        m_columns = columns;
    }
    if (rows > 0) {
        m_rows = rows;
    }
}

void ProcessTerminalBackend::stop()
{
    if (m_stopping) {
        return;
    }
    m_stopping = true;
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

bool ProcessTerminalBackend::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void ProcessTerminalBackend::onReadyRead()
{
    if (m_stopping) {
        return;
    }
    const QByteArray output = m_process->readAllStandardOutput();
    const QByteArray error = m_process->readAllStandardError();
    if (!output.isEmpty()) {
        emit dataReceived(output, false);
    }
    if (!error.isEmpty()) {
        emit dataReceived(error, true);
    }
}
