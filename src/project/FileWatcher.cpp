#include "project/FileWatcher.h"

#include "core/Logging.h"

#include <QDir>
#include <QFileInfo>

namespace {

QString normalize(const QString &path)
{
    if (path.isEmpty()) {
        return {};
    }
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    if (!canonical.isEmpty()) {
        return canonical;
    }
    return QDir::cleanPath(info.absoluteFilePath());
}

} // namespace

FileWatcher::FileWatcher(QObject *parent)
    : QObject(parent)
{
    m_dirDebounce.setSingleShot(true);
    m_dirDebounce.setInterval(250);
    connect(&m_dirDebounce, &QTimer::timeout, this, &FileWatcher::rootContentsChanged);

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &FileWatcher::onDirectoryChanged);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged,
            this, &FileWatcher::onFileChanged);
}

void FileWatcher::setRootPath(const QString &path)
{
    const QString normalized = normalize(path);
    if (normalized == m_rootPath) {
        return;
    }

    for (const QString &dir : m_watcher.directories()) {
        m_watcher.removePath(dir);
    }

    m_rootPath = normalized;
    if (!m_rootPath.isEmpty() && QFileInfo::exists(m_rootPath)) {
        watchDirectory(m_rootPath);
    }
}

void FileWatcher::watchDirectory(const QString &path)
{
    const QString normalized = normalize(path);
    if (normalized.isEmpty() || !QFileInfo(normalized).isDir()) {
        return;
    }
    if (m_watcher.directories().contains(normalized)) {
        return;
    }
    if (!m_watcher.addPath(normalized)) {
        qCWarning(lcProject) << "Could not watch directory" << normalized;
    }
}

void FileWatcher::setFilePaths(const QStringList &paths)
{
    const QStringList current = m_watcher.files();
    for (const QString &file : current) {
        m_watcher.removePath(file);
    }

    for (const QString &path : paths) {
        const QString normalized = normalize(path);
        if (normalized.isEmpty() || !QFileInfo::exists(normalized) || QFileInfo(normalized).isDir()) {
            continue;
        }
        if (!m_watcher.addPath(normalized)) {
            qCWarning(lcProject) << "Could not watch file" << normalized;
        }
    }
}

void FileWatcher::ignoreNextChange(const QString &path)
{
    const QString normalized = normalize(path);
    if (!normalized.isEmpty()) {
        m_ignoreOnce.insert(normalized);
    }
}

void FileWatcher::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)
    m_dirDebounce.start();
}

void FileWatcher::onFileChanged(const QString &path)
{
    const QString normalized = normalize(path);
    rewatch(normalized);

    if (m_ignoreOnce.remove(normalized)) {
        return;
    }
    emit fileChangedOnDisk(normalized);
}

void FileWatcher::rewatch(const QString &path)
{
    if (path.isEmpty()) {
        return;
    }
    if (m_watcher.files().contains(path) || m_watcher.directories().contains(path)) {
        return;
    }
    if (QFileInfo::exists(path)) {
        m_watcher.addPath(path);
    }
}
