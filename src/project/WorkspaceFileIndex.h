#ifndef EDITERAKO_WORKSPACEFILEINDEX_H
#define EDITERAKO_WORKSPACEFILEINDEX_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <mutex>
#include <thread>

class WorkspaceFileIndex : public QObject
{
    Q_OBJECT

public:
    explicit WorkspaceFileIndex(QObject *parent = nullptr);
    ~WorkspaceFileIndex() override;

    void setRootPath(const QString &path);
    void setExcludedNames(const QStringList &names);
    void rebuild();

    [[nodiscard]] QString rootPath() const { return m_rootPath; }
    [[nodiscard]] QStringList files() const;
    [[nodiscard]] bool isIndexing() const { return m_pending > 0; }

signals:
    void indexUpdated();

private:
    void startJob();
    void joinWorker();
    void applyResult(quint64 generation);

    QString m_rootPath;
    QStringList m_excludedNames;
    QStringList m_files;
    QStringList m_handoff;
    std::mutex m_mutex;
    std::thread m_thread;
    quint64 m_generation = 0;
    bool m_rebuildQueued = false;
    bool m_destroying = false;
    int m_pending = 0;
};

[[nodiscard]] QStringList collectWorkspaceFiles(const QString &root,
                                                const QStringList &excludedNames,
                                                int maxFiles = 50000);

#endif
