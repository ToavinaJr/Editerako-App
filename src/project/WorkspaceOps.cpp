#include "project/WorkspaceOps.h"

#include "project/WorkspacePath.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace {

bool copyRecursively(const QString &source, const QString &destination)
{
    const QFileInfo info(source);
    if (info.isDir()) {
        if (!QDir().mkpath(destination)) {
            return false;
        }
        const QFileInfoList entries = QDir(source).entryInfoList(
            QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
        for (const QFileInfo &entry : entries) {
            const QString dest = QDir(destination).filePath(entry.fileName());
            if (!copyRecursively(entry.absoluteFilePath(), dest)) {
                return false;
            }
        }
        return true;
    }
    return QFile::copy(source, destination);
}

bool removeRecursively(const QString &path)
{
    const QFileInfo info(path);
    if (info.isDir()) {
        return QDir(path).removeRecursively();
    }
    return QFile::remove(path);
}

QString destinationPath(const QString &source, const QString &destinationDirectory)
{
    return QDir::cleanPath(QDir(destinationDirectory).filePath(QFileInfo(source).fileName()));
}

} // namespace

QString uniqueSiblingPath(const QString &path)
{
    const QFileInfo info(path);
    const QString dir = info.absolutePath();
    const QString fileName = info.fileName();
    const int dot = info.isDir() ? -1 : fileName.lastIndexOf(QLatin1Char('.'));
    const QString stem = (dot > 0) ? fileName.left(dot) : fileName;
    const QString ext = (dot > 0) ? fileName.mid(dot) : QString();
    QString candidate = QDir(dir).filePath(stem + QStringLiteral(" copy") + ext);
    int n = 2;
    while (QFileInfo::exists(candidate)) {
        candidate = QDir(dir).filePath(stem + QStringLiteral(" copy %1").arg(n) + ext);
        ++n;
    }
    return QDir::cleanPath(candidate);
}

bool renameInsideWorkspace(const QString &workspaceRoot,
                           const QString &path,
                           const QString &newName,
                           QString *resultPath)
{
    if (newName.contains(QLatin1Char('/')) || newName.contains(QLatin1Char('\\'))) {
        return false;
    }
    if (!isInsideWorkspace(workspaceRoot, path)) {
        return false;
    }
    const QString dest = resolveInsideWorkspace(workspaceRoot, QFileInfo(path).absolutePath(), newName);
    if (dest.isEmpty() || QFileInfo::exists(dest)) {
        return false;
    }
    if (QDir::cleanPath(path) == QDir::cleanPath(workspaceRoot)) {
        return false;
    }
    if (!QFile::rename(path, dest)) {
        return false;
    }
    if (!isInsideWorkspace(workspaceRoot, dest)) {
        QFile::rename(dest, path);
        return false;
    }
    if (resultPath) {
        *resultPath = dest;
    }
    return true;
}

bool deleteInsideWorkspace(const QString &workspaceRoot,
                           const QString &path,
                           WorkspaceDeleteMode mode)
{
    if (!isInsideWorkspace(workspaceRoot, path)) {
        return false;
    }
    if (QDir::cleanPath(normalizePath(path)) == QDir::cleanPath(normalizePath(workspaceRoot))) {
        return false;
    }
    if (mode == WorkspaceDeleteMode::Trash) {
        return QFile::moveToTrash(path);
    }
    return removeRecursively(path);
}

bool duplicateInsideWorkspace(const QString &workspaceRoot,
                              const QString &path,
                              QString *resultPath)
{
    if (!isInsideWorkspace(workspaceRoot, path)) {
        return false;
    }
    const QString dest = uniqueSiblingPath(path);
    if (!isInsideWorkspace(workspaceRoot, dest)) {
        return false;
    }
    if (!copyRecursively(path, dest)) {
        return false;
    }
    if (resultPath) {
        *resultPath = dest;
    }
    return true;
}

bool copyInsideWorkspace(const QString &workspaceRoot,
                         const QString &source,
                         const QString &destinationDirectory,
                         QString *resultPath)
{
    if (!isInsideWorkspace(workspaceRoot, source)
        || !isInsideWorkspace(workspaceRoot, destinationDirectory)) {
        return false;
    }
    QString dest = destinationPath(source, destinationDirectory);
    if (QDir::cleanPath(source) == dest) {
        dest = uniqueSiblingPath(dest);
    } else if (QFileInfo::exists(dest)) {
        dest = uniqueSiblingPath(dest);
    }
    if (!isInsideWorkspace(workspaceRoot, dest)) {
        return false;
    }
    if (!copyRecursively(source, dest)) {
        return false;
    }
    if (resultPath) {
        *resultPath = dest;
    }
    return true;
}

bool moveInsideWorkspace(const QString &workspaceRoot,
                         const QString &source,
                         const QString &destinationDirectory,
                         QString *resultPath)
{
    if (!isInsideWorkspace(workspaceRoot, source)
        || !isInsideWorkspace(workspaceRoot, destinationDirectory)) {
        return false;
    }
    if (QDir::cleanPath(normalizePath(source)) == QDir::cleanPath(normalizePath(workspaceRoot))) {
        return false;
    }
    QString dest = destinationPath(source, destinationDirectory);
    if (QDir::cleanPath(source) == dest) {
        return false;
    }
    if (QFileInfo::exists(dest)) {
        dest = uniqueSiblingPath(dest);
    }
    if (!isInsideWorkspace(workspaceRoot, dest)) {
        return false;
    }
    if (QFile::rename(source, dest)) {
        if (resultPath) {
            *resultPath = dest;
        }
        return true;
    }
    if (!copyRecursively(source, dest)) {
        return false;
    }
    if (!removeRecursively(source)) {
        return false;
    }
    if (resultPath) {
        *resultPath = dest;
    }
    return true;
}
