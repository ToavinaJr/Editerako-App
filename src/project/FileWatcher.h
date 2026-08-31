#ifndef EDITERAKO_FILEWATCHER_H
#define EDITERAKO_FILEWATCHER_H

#include <QFileSystemWatcher>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>

class FileWatcher : public QObject
{
    Q_OBJECT

public:
    explicit FileWatcher(QObject *parent = nullptr);

    void setRootPath(const QString &path);
    void watchDirectory(const QString &path);
    void setFilePaths(const QStringList &paths);
    void ignoreNextChange(const QString &path);

signals:
    void rootContentsChanged();
    void fileChangedOnDisk(const QString &path);

private:
    void onDirectoryChanged(const QString &path);
    void onFileChanged(const QString &path);
    void rewatch(const QString &path);

    QFileSystemWatcher m_watcher;
    QTimer m_dirDebounce;
    QString m_rootPath;
    QSet<QString> m_ignoreOnce;
};

#endif
