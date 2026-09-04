#include "tasks/TaskRunner.h"

#include "core/Logging.h"

#include <QFileInfo>
#include <QProcess>

TaskRunner::TaskRunner(QObject *parent)
    : QObject(parent)
{
}

TaskRunner::~TaskRunner()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
    }
}

bool TaskRunner::isRunning() const
{
    return m_running;
}

void TaskRunner::start(const ProcessSpec &spec)
{
    if (m_running) {
        emit failed(QStringLiteral("A task is already running"));
        return;
    }
    if (spec.program.isEmpty()) {
        emit failed(QStringLiteral("No program to run"));
        return;
    }

    if (spec.detach) {
        const bool ok = QProcess::startDetached(spec.program, spec.arguments, spec.workingDirectory);
        if (!ok) {
            emit failed(QStringLiteral("Failed to start %1").arg(spec.program));
            return;
        }
        emit started(spec.title);
        emit outputReceived(QStringLiteral("> %1\n").arg(spec.program));
        emit finished(0, {});
        return;
    }

    if (!m_process) {
        m_process = new QProcess(this);
        m_process->setProcessChannelMode(QProcess::MergedChannels);
        connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
            const QString chunk = QString::fromLocal8Bit(m_process->readAllStandardOutput());
            m_output += chunk;
            emit outputReceived(chunk);
        });
        connect(m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus) {
            emitFinished(exitCode);
        });
        connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                emitFinished(-1);
            }
        });
    }

    m_output.clear();
    m_running = true;
    m_process->setWorkingDirectory(spec.workingDirectory);
    emit started(spec.title);
    const QString header = QStringLiteral("> %1 %2\n")
                               .arg(QFileInfo(spec.program).fileName(),
                                    spec.arguments.join(QLatin1Char(' ')));
    m_output += header;
    emit outputReceived(header);
    m_process->start(spec.program, spec.arguments);
}

void TaskRunner::cancel()
{
    if (!m_running || !m_process) {
        return;
    }
    qCInfo(lcTasks) << "Cancelling task";
    m_process->kill();
}

void TaskRunner::emitFinished(int exitCode)
{
    if (!m_running) {
        return;
    }
    m_running = false;
    if (m_process && m_process->bytesAvailable() > 0) {
        const QString chunk = QString::fromLocal8Bit(m_process->readAllStandardOutput());
        m_output += chunk;
        emit outputReceived(chunk);
    }
    if (exitCode == -1 && m_process && m_process->error() == QProcess::FailedToStart) {
        emit failed(m_process->errorString());
        return;
    }
    emit finished(exitCode, m_output);
}
