#include "scm/GitParsers.h"

#include <QDir>
#include <QFileInfo>
#include <QHash>

namespace {

ScmFileState stateFor(char code)
{
    switch (code) {
    case 'M': return ScmFileState::Modified;
    case 'A': return ScmFileState::Added;
    case 'D': return ScmFileState::Deleted;
    case 'R': return ScmFileState::Renamed;
    case 'C': return ScmFileState::Copied;
    case '?': return ScmFileState::Untracked;
    case 'U': return ScmFileState::Conflicted;
    default: return ScmFileState::Unknown;
    }
}

void appendChange(ScmStatus &status, const QByteArray &path, char code, bool staged,
                  const QByteArray &oldPath = {})
{
    if (path.isEmpty() || code == ' ' || code == '!') {
        return;
    }
    ScmChange change;
    change.path = QString::fromUtf8(path);
    change.oldPath = QString::fromUtf8(oldPath);
    change.state = stateFor(code);
    change.staged = staged;
    status.changes.append(change);
}

} // namespace

ScmStatus GitParsers::parseStatus(const QByteArray &output)
{
    ScmStatus status;
    status.isRepository = true;
    const QList<QByteArray> records = output.split('\0');
    for (qsizetype i = 0; i < records.size(); ++i) {
        const QByteArray &record = records.at(i);
        if (record.startsWith("## ")) {
            QByteArray branch = record.mid(3);
            const qsizetype separator = branch.indexOf("...");
            if (separator >= 0) branch.truncate(separator);
            status.branch = QString::fromUtf8(branch);
            continue;
        }
        if (record.size() < 4 || record.at(2) != ' ') continue;
        const char indexState = record.at(0);
        const char workTreeState = record.at(1);
        QByteArray path = record.mid(3);
        QByteArray oldPath;
        if ((indexState == 'R' || indexState == 'C' || workTreeState == 'R' || workTreeState == 'C')
            && i + 1 < records.size()) {
            oldPath = records.at(++i);
        }
        if (indexState == '?' && workTreeState == '?') {
            appendChange(status, path, '?', false);
            continue;
        }
        appendChange(status, path, indexState, true, oldPath);
        appendChange(status, path, workTreeState, false, oldPath);
    }
    return status;
}

QString GitParsers::parseRepositoryRoot(const QByteArray &output)
{
    return QDir::fromNativeSeparators(QString::fromUtf8(output).trimmed());
}

void GitParsers::makePathsAbsolute(ScmStatus &status, const QString &repositoryRoot)
{
    status.repositoryRoot = QDir::cleanPath(repositoryRoot);
    if (status.repositoryRoot.isEmpty()) {
        return;
    }
    const QDir root(status.repositoryRoot);
    for (ScmChange &change : status.changes) {
        if (!change.path.isEmpty() && !QFileInfo(change.path).isAbsolute()) {
            change.path = QDir::cleanPath(root.filePath(change.path));
        }
        if (!change.oldPath.isEmpty() && !QFileInfo(change.oldPath).isAbsolute()) {
            change.oldPath = QDir::cleanPath(root.filePath(change.oldPath));
        }
    }
}

QString GitParsers::badgeFor(ScmFileState state)
{
    switch (state) {
    case ScmFileState::Modified:
        return QStringLiteral("M");
    case ScmFileState::Added:
        return QStringLiteral("A");
    case ScmFileState::Deleted:
        return QStringLiteral("D");
    case ScmFileState::Renamed:
        return QStringLiteral("R");
    case ScmFileState::Copied:
        return QStringLiteral("C");
    case ScmFileState::Untracked:
        return QStringLiteral("U");
    case ScmFileState::Conflicted:
        return QStringLiteral("!");
    case ScmFileState::Unknown:
        return QStringLiteral("?");
    }
    return QStringLiteral("?");
}

QHash<QString, QString> GitParsers::explorerBadges(const ScmStatus &status)
{
    QHash<QString, QString> badges;
    for (const ScmChange &change : status.changes) {
        if (change.path.isEmpty()) {
            continue;
        }
        badges.insert(QDir::cleanPath(change.path), badgeFor(change.state));
    }
    return badges;
}

QString GitParsers::branchName(const ScmStatus &status)
{
    if (!status.isRepository) {
        return {};
    }
    return status.branch;
}

