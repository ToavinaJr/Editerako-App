#include "project/WorkspaceFileIndex.h"

#include "core/Logging.h"

#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
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
}

WorkspaceFileIndex::~WorkspaceFileIndex()
{
    m_destroying = true;
    m_rebuildQueued = false;
    ++m_generation;
    joinWorker();
}

void WorkspaceFileIndex::joinWorker()
{
    if (m_thread.joinable()) {
        m_thread.join();
    }
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
    if (m_pending > 0) {
        m_rebuildQueued = true;
        return;
    }
    startJob();
}

void WorkspaceFileIndex::startJob()
{
    joinWorker();
    m_rebuildQueued = false;
    m_pending = 1;
    const quint64 generation = ++m_generation;
    const QString root = m_rootPath;
    const QStringList excluded = m_excludedNames;
    m_thread = std::thread([this, generation, root, excluded]() {
        QStringList files = collectWorkspaceFiles(root, excluded);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_handoff = std::move(files);
        }
        QMetaObject::invokeMethod(this, [this, generation]() { applyResult(generation); },
                                  Qt::QueuedConnection);
    });
}

void WorkspaceFileIndex::applyResult(quint64 generation)
{
    if (m_destroying || generation != m_generation) {
        return;
    }
    if (m_rebuildQueued) {
        startJob();
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_files.swap(m_handoff);
        m_handoff.clear();
    }
    m_pending = 0;
    qCInfo(lcProject) << "Indexed" << m_files.size() << "files under" << m_rootPath;
    emit indexUpdated();
}

QStringList WorkspaceFileIndex::files() const
{
    return m_files;
}
