#include "scm/GitCliProvider.h"

#include "scm/GitParsers.h"

#include <QDir>
#include <QProcess>

GitCliProvider::GitCliProvider(QObject *parent)
    : ISourceControlProvider(parent), m_process(new QProcess(this))
{
    connect(m_process, &QProcess::finished, this,
            [this](int code, QProcess::ExitStatus) { handleFinished(code); });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) return;
        emit operationFailed({tr("Git could not be started. Verify that it is installed and on PATH."), -1});
        m_queue.clear();
        emit busyChanged(false);
    });
}

void GitCliProvider::setWorkspace(const QString &path)
{
    m_workspace = QDir::cleanPath(path);
    m_queue.clear();
    if (!m_workspace.isEmpty()) refresh();
}

void GitCliProvider::refresh()
{
    enqueue({{QStringLiteral("status"), QStringLiteral("--porcelain=v1"),
              QStringLiteral("--branch"), QStringLiteral("-z")}, {}, false});
}

void GitCliProvider::stage(const QStringList &paths)
{
    if (paths.isEmpty()) return;
    enqueue({QStringList{QStringLiteral("add"), QStringLiteral("--")} + paths, {}, true});
}

void GitCliProvider::unstage(const QStringList &paths)
{
    if (paths.isEmpty()) return;
    enqueue({QStringList{QStringLiteral("restore"), QStringLiteral("--staged"), QStringLiteral("--")} + paths, {}, true});
}

void GitCliProvider::discard(const QStringList &paths)
{
    if (paths.isEmpty()) return;
    enqueue({QStringList{QStringLiteral("restore"), QStringLiteral("--worktree"), QStringLiteral("--")} + paths, {}, true});
}

void GitCliProvider::commit(const QString &message)
{
    if (!message.trimmed().isEmpty()) enqueue({{QStringLiteral("commit"), QStringLiteral("-m"), message}, {}, true});
}

void GitCliProvider::requestDiff(const QString &path, bool staged)
{
    QStringList args{QStringLiteral("diff"), QStringLiteral("--no-color")};
    if (staged) args << QStringLiteral("--cached");
    args << QStringLiteral("--") << path;
    enqueue({args, path, false});
}

void GitCliProvider::enqueue(Command command)
{
    if (m_workspace.isEmpty()) return;
    m_queue.enqueue(std::move(command));
    startNext();
}

void GitCliProvider::startNext()
{
    if (m_process->state() != QProcess::NotRunning || m_queue.isEmpty()) return;
    m_current = m_queue.dequeue();
    m_process->setWorkingDirectory(m_workspace);
    emit busyChanged(true);
    m_process->start(QStringLiteral("git"), m_current.arguments);
}

void GitCliProvider::handleFinished(int exitCode)
{
    const QByteArray output = m_process->readAllStandardOutput();
    const QString error = QString::fromUtf8(m_process->readAllStandardError()).trimmed();
    if (exitCode != 0) {
        ScmStatus status;
        status.isRepository = false;
        if (m_current.arguments.value(0) == QStringLiteral("status")) emit statusChanged(status);
        else emit operationFailed({error.isEmpty() ? tr("Git command failed.") : error, exitCode});
    } else if (m_current.arguments.value(0) == QStringLiteral("status")) {
        ScmStatus status = GitParsers::parseStatus(output);
        status.repositoryRoot = m_workspace;
        emit statusChanged(status);
    } else if (!m_current.diffPath.isEmpty()) {
        emit diffReady(m_current.diffPath, QString::fromUtf8(output));
    }
    const bool refreshAfter = exitCode == 0 && m_current.refreshAfter;
    if (refreshAfter) m_queue.prepend({{QStringLiteral("status"), QStringLiteral("--porcelain=v1"), QStringLiteral("--branch"), QStringLiteral("-z")}, {}, false});
    emit busyChanged(!m_queue.isEmpty());
    startNext();
}

