#include "project/Workspace.h"

#include "core/AppSettings.h"
#include "core/Logging.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <algorithm>

Workspace::Workspace(QObject *parent)
    : QObject(parent)
{
}

void Workspace::setRootPath(const QString &path)
{
    const QString normalized = path.isEmpty()
        ? QString()
        : QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    if (m_rootPath == normalized) {
        return;
    }
    m_rootPath = normalized;
    qCInfo(lcProject) << "Workspace root" << m_rootPath;
    emit rootPathChanged(m_rootPath);
}

bool Workspace::isValid() const
{
    return !m_rootPath.isEmpty() && QDir(m_rootPath).exists();
}

QStringList Workspace::excludedNames() const
{
    return AppSettings().excludedFolders();
}

bool Workspace::isExcludedName(const QString &name) const
{
    if (name.isEmpty()) {
        return false;
    }
    const QStringList excluded = excludedNames();
    return std::any_of(excluded.cbegin(), excluded.cend(), [&](const QString &entry) {
        return name.compare(entry, Qt::CaseInsensitive) == 0;
    });
}

bool Workspace::containsPath(const QString &filePath) const
{
    if (m_rootPath.isEmpty() || filePath.isEmpty()) {
        return false;
    }
    const QString root = QDir::cleanPath(m_rootPath);
    const QString abs = QDir::cleanPath(QFileInfo(filePath).absoluteFilePath());
#ifdef Q_OS_WIN
    constexpr auto kCs = Qt::CaseInsensitive;
#else
    constexpr auto kCs = Qt::CaseSensitive;
#endif
    if (abs.compare(root, kCs) == 0) {
        return true;
    }
    const QString prefix = root.endsWith(QLatin1Char('/')) ? root : root + QLatin1Char('/');
    return abs.startsWith(prefix, kCs);
}

QList<Workspace::Entry> Workspace::listEntries(const QString &directoryPath) const
{
    QList<Entry> result;
    QDir dir(directoryPath);
    if (!dir.exists()) {
        return result;
    }

    const QFileInfoList infos = dir.entryInfoList(
        QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden,
        QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);

    result.reserve(infos.size());
    for (const QFileInfo &info : infos) {
        if (isExcludedName(info.fileName())) {
            continue;
        }
        Entry entry;
        entry.name = info.fileName();
        entry.absolutePath = info.absoluteFilePath();
        entry.isDirectory = info.isDir();
        result.append(entry);
    }
    return result;
}

bool Workspace::createEmptyFile(const QString &directory, const QString &fileName, QString *absolutePath)
{
    if (fileName.isEmpty()) {
        return false;
    }
    const QString fullPath = QDir(directory).absoluteFilePath(fileName);
    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.close();
    if (absolutePath) {
        *absolutePath = fullPath;
    }
    return true;
}

bool Workspace::createDirectory(const QString &directory, const QString &folderName, QString *absolutePath)
{
    if (folderName.isEmpty()) {
        return false;
    }
    const QString fullPath = QDir(directory).absoluteFilePath(folderName);
    QDir dir;
    if (!dir.mkpath(fullPath)) {
        return false;
    }
    if (absolutePath) {
        *absolutePath = fullPath;
    }
    return true;
}
