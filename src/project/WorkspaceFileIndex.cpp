#include "project/WorkspaceFileIndex.h"

#include "core/Logging.h"

#include <QDir>
#include <QFileInfo>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>
#include <algorithm>

namespace {

bool isExcludedName(const QString &name, const QStringList &excluded)
{
    return std::any_of(excluded.cbegin(), excluded.cend(), [&](const QString &entry) {
        return name.compare(entry, Qt::CaseInsensitive) == 0;
    });
}

} // namespace

QStringList collectWorkspaceFiles(const QString &root, const QStringList &excludedNames, int maxFiles)
{
    QStringList files;
    if (root.isEmpty() || !QDir(root).exists()) {
        return files;
    }

    QStringList stack{QDir::cleanPath(root)};
    while (!stack.isEmpty() && files.size() < maxFiles) {
        const QString dirPath = stack.takeLast();
        const QFileInfoList infos = QDir(dirPath).entryInfoList(
            QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden,
            QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);
        for (const QFileInfo &info : infos) {
            if (isExcludedName(info.fileName(), excludedNames)) {
                continue;
            }
            if (info.isDir()) {
                stack.append(info.absoluteFilePath());
                continue;
            }
            files.append(QDir::cleanPath(info.absoluteFilePath()));
            if (files.size() >= maxFiles) {
                break;
            }
        }
    }
    return files;
}

WorkspaceFileIndex::WorkspaceFileIndex(QObject *parent)
    : QObject(parent)
{
    connect(&m_watcher, &QFutureWatcher<QStringList>::finished, this, [this]() {
        if (m_destroying) {
            return;
        }
        if (m_rebuildQueued) {
            startJob();
            return;
        }
        m_files = m_watcher.result();
        m_pending = 0;
        qCInfo(lcProject) << "Indexed" << m_files.size() << "files under" << m_rootPath;
        emit indexUpdated();
    });
}

WorkspaceFileIndex::~WorkspaceFileIndex()
{
    m_destroying = true;
    m_rebuildQueued = false;
    m_watcher.disconnect();
    if (m_watcher.future().isValid()) {
        m_watcher.waitForFinished();
    }
    QThreadPool::globalInstance()->waitForDone();
    m_watcher.setFuture({});
}

void WorkspaceFileIndex::setRootPath(const QString &path)
{
    m_rootPath = path.isEmpty() ? QString() : QDir::cleanPath(path);
}

void WorkspaceFileIndex::setExcludedNames(const QStringList &names)
{
    m_excludedNames = names;
}

void WorkspaceFileIndex::rebuild()
{
    if (m_watcher.isRunning()) {
        m_rebuildQueued = true;
        return;
    }
    startJob();
}

void WorkspaceFileIndex::startJob()
{
    m_rebuildQueued = false;
    m_pending = 1;
    const QString root = m_rootPath;
    const QStringList excluded = m_excludedNames;
    m_watcher.setFuture(QtConcurrent::run([root, excluded]() {
        return collectWorkspaceFiles(root, excluded);
    }));
}

QStringList WorkspaceFileIndex::files() const
{
    return m_files;
}
