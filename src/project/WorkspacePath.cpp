#include "project/WorkspacePath.h"

#include <QDir>
#include <QFileInfo>

namespace {

#ifdef Q_OS_WIN
constexpr auto kCs = Qt::CaseInsensitive;
#else
constexpr auto kCs = Qt::CaseSensitive;
#endif

bool isPrefixPath(const QString &root, const QString &path)
{
    if (path.compare(root, kCs) == 0) {
        return true;
    }
    const QString prefix = root.endsWith(QLatin1Char('/')) ? root : root + QLatin1Char('/');
    return path.startsWith(prefix, kCs);
}

} // namespace

bool isSafeRelativePath(const QString &name)
{
    if (name.isEmpty() || name.size() > 512 || name.contains(QChar(QChar::Null))) {
        return false;
    }
    const QString n = QDir::fromNativeSeparators(name.trimmed());
    if (n.startsWith(QLatin1Char('/'))) {
        return false;
    }
    if (QFileInfo(n).isAbsolute()) {
        return false;
    }
#ifdef Q_OS_WIN
    if (n.size() >= 2 && n.at(1) == QLatin1Char(':')) {
        return false;
    }
#endif
    const QStringList parts = n.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return false;
    }
    for (const QString &part : parts) {
        if (part == QLatin1String("..")) {
            return false;
        }
    }
    return true;
}

QString normalizePath(const QString &path)
{
    if (path.isEmpty()) {
        return {};
    }
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    if (!canonical.isEmpty()) {
        return QDir::cleanPath(canonical);
    }
    return QDir::cleanPath(info.absoluteFilePath());
}

bool isInsideWorkspace(const QString &workspaceRoot, const QString &path)
{
    if (workspaceRoot.isEmpty() || path.isEmpty()) {
        return false;
    }
    const QString rootAbs = QDir::cleanPath(QFileInfo(workspaceRoot).absoluteFilePath());
    const QString pathAbs = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    if (rootAbs.isEmpty() || pathAbs.isEmpty() || !isPrefixPath(rootAbs, pathAbs)) {
        return false;
    }

    const QString rootCanon = QFileInfo(rootAbs).canonicalFilePath();
    const QString pathCanon = QFileInfo(pathAbs).canonicalFilePath();
    if (!rootCanon.isEmpty() && !pathCanon.isEmpty()) {
        return isPrefixPath(QDir::cleanPath(rootCanon), QDir::cleanPath(pathCanon));
    }
    return true;
}

QString resolveInsideWorkspace(const QString &workspaceRoot,
                               const QString &directory,
                               const QString &relativeName)
{
    if (!isSafeRelativePath(relativeName) || !isInsideWorkspace(workspaceRoot, directory)) {
        return {};
    }
    const QString intended = QDir::cleanPath(QDir(directory).absoluteFilePath(relativeName));
    if (!isInsideWorkspace(workspaceRoot, intended)) {
        return {};
    }
    return intended;
}
