#include "scm/GitCliProvider.h"

#include "core/Logging.h"
#include "scm/GitParsers.h"
#include "scm/GitProcess.h"
#include "scm/TextDiff.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>

namespace {

QStringList statusArgs()
{
    return {QStringLiteral("status"), QStringLiteral("--porcelain=v1"), QStringLiteral("--branch"),
            QStringLiteral("-z")};
}

struct GitJobResult {
    GitRunResult primary;
    QString repositoryRoot;
    QString synthesizedDiff;
};

GitJobResult runJob(const QString &cwd, const QStringList &args, bool refresh, bool untrackedDiff,
                    const QString &diffPath)
{
    GitJobResult job;
    if (refresh) {
        job.primary = GitProcess::run(cwd, args);
        if (job.primary.ok()) {
            const GitRunResult root =
                GitProcess::run(cwd, {QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel")});
            if (root.ok()) {
                job.repositoryRoot = GitParsers::parseRepositoryRoot(root.standardOutput);
            }
        }
        return job;
    }
    if (untrackedDiff) {
        QFile file(diffPath);
        if (file.open(QIODevice::ReadOnly)) {
            const QString text = QString::fromUtf8(file.readAll());
            job.synthesizedDiff =
                TextDiff::unified({}, text, QStringLiteral("/dev/null"), QFileInfo(diffPath).fileName());
            job.primary.exitCode = 0;
        } else {
            job.primary = GitProcess::run(cwd, args);
        }
        return job;
    }
    job.primary = GitProcess::run(cwd, args);
    return job;
}

} // namespace

GitCliProvider::GitCliProvider(QObject *parent)
    : ISourceControlProvider(parent)
{
    qRegisterMetaType<ScmStatus>();
    qRegisterMetaType<ScmError>();
}

GitCliProvider::~GitCliProvider()
{
    ++m_generation;
    m_queue.clear();
    joinWorker();
}

void GitCliProvider::joinWorker()
{
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

void GitCliProvider::setWorkspace(const QString &path)
{
    ++m_generation;
    m_queue.clear();
    m_workspace = QDir::cleanPath(path);
    m_status = {};
    if (m_workspace.isEmpty()) {
        emit statusChanged(m_status);
        emit busyChanged(false);
        return;
    }
    refresh();
}

void GitCliProvider::refresh()
{
    enqueue({Kind::Refresh, statusArgs(), {}, false, false});
}

void GitCliProvider::stage(const QStringList &paths)
{
    if (paths.isEmpty()) {
        return;
    }
    enqueue({Kind::Mutate, QStringList{QStringLiteral("add"), QStringLiteral("--")} + paths, {}, true, false});
}

void GitCliProvider::unstage(const QStringList &paths)
{
    if (paths.isEmpty()) {
        return;
    }
    enqueue({Kind::Mutate,
             QStringList{QStringLiteral("restore"), QStringLiteral("--staged"), QStringLiteral("--")} + paths,
             {},
             true,
             false});
}

void GitCliProvider::discard(const QStringList &paths)
{
    QStringList restorePaths;
    QStringList cleanPaths;
    for (const QString &path : paths) {
        if (path.isEmpty()) {
            continue;
        }
        if (isUntracked(path)) {
            cleanPaths.append(path);
        } else {
            restorePaths.append(path);
        }
    }
    if (!restorePaths.isEmpty()) {
        enqueue({Kind::Mutate,
                 QStringList{QStringLiteral("restore"), QStringLiteral("--worktree"), QStringLiteral("--")}
                     + restorePaths,
                 {},
                 true,
                 false});
    }
    if (!cleanPaths.isEmpty()) {
        enqueue({Kind::Mutate,
                 QStringList{QStringLiteral("clean"), QStringLiteral("-f"), QStringLiteral("--")} + cleanPaths,
                 {},
                 true,
                 false});
    }
}

void GitCliProvider::commit(const QString &message)
{
    const QString trimmed = message.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    enqueue({Kind::Mutate, {QStringLiteral("commit"), QStringLiteral("-m"), trimmed}, {}, true, false});
}

void GitCliProvider::requestDiff(const QString &path, bool staged)
{
    if (path.isEmpty()) {
        return;
    }
    QStringList args{QStringLiteral("diff"), QStringLiteral("--no-color")};
    if (staged) {
        args << QStringLiteral("--cached");
    }
    args << QStringLiteral("--") << path;
    enqueue({Kind::Diff, args, path, false, !staged && isUntracked(path)});
}

void GitCliProvider::enqueue(Command command)
{
    if (m_workspace.isEmpty()) {
        return;
    }
    m_queue.enqueue(std::move(command));
    startNext();
}

void GitCliProvider::startNext()
{
    if (m_running || m_queue.isEmpty()) {
        return;
    }

    joinWorker();
    m_current = m_queue.dequeue();
    m_running = true;
    emit busyChanged(true);

    const quint64 generation = m_generation;
    const QString cwd = m_workspace;
    const Command job = m_current;
    m_worker = std::thread([this, cwd, job, generation]() {
        GitJobResult result =
            runJob(cwd, job.arguments, job.kind == Kind::Refresh, job.untrackedDiff, job.diffPath);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_handoffPrimary = std::move(result.primary);
            m_handoffRoot = std::move(result.repositoryRoot);
            m_handoffDiff = std::move(result.synthesizedDiff);
        }
        QMetaObject::invokeMethod(this, [this, generation]() { deliverResult(generation); },
                                  Qt::QueuedConnection);
    });
}

void GitCliProvider::deliverResult(quint64 generation)
{
    GitRunResult primary;
    QString repositoryRoot;
    QString synthesizedDiff;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        primary = std::move(m_handoffPrimary);
        repositoryRoot = std::move(m_handoffRoot);
        synthesizedDiff = std::move(m_handoffDiff);
    }
    if (generation != m_generation) {
        m_running = false;
        emit busyChanged(!m_queue.isEmpty());
        startNext();
        return;
    }
    m_running = false;

    if (primary.error == QLatin1String("git-not-found")) {
        qCWarning(lcScm) << "Git executable not found";
        m_status = {};
        emit statusChanged(m_status);
        emit operationFailed({tr("Git was not found on PATH."), -1});
        m_queue.clear();
        emit busyChanged(false);
        return;
    }

    if (m_current.kind == Kind::Refresh) {
        if (!primary.ok()) {
            m_status = {};
            emit statusChanged(m_status);
        } else {
            m_status = GitParsers::parseStatus(primary.standardOutput);
            GitParsers::makePathsAbsolute(m_status,
                                          repositoryRoot.isEmpty() ? m_workspace : repositoryRoot);
            emit statusChanged(m_status);
        }
    } else if (m_current.kind == Kind::Diff) {
        const QString text =
            synthesizedDiff.isEmpty() ? QString::fromUtf8(primary.standardOutput) : synthesizedDiff;
        emit diffReady(m_current.diffPath, text);
    } else if (!primary.ok()) {
        emit operationFailed({primary.stderrText().isEmpty() ? tr("Git command failed.")
                                                            : primary.stderrText(),
                             primary.exitCode});
    }

    if (primary.ok() && m_current.refreshAfter) {
        m_queue.prepend({Kind::Refresh, statusArgs(), {}, false, false});
    }
    emit busyChanged(!m_queue.isEmpty());
    startNext();
}

bool GitCliProvider::isUntracked(const QString &path) const
{
    const QString clean = QDir::cleanPath(path);
    for (const ScmChange &change : m_status.changes) {
        if (change.state != ScmFileState::Untracked) {
            continue;
        }
        if (QDir::cleanPath(change.path) == clean || change.path == path) {
            return true;
        }
    }
    return false;
}
