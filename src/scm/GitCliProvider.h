#ifndef EDITERAKO_GITCLIPROVIDER_H
#define EDITERAKO_GITCLIPROVIDER_H

#include "scm/ISourceControlProvider.h"

#include <QQueue>
#include <QStringList>

class GitCliProvider final : public ISourceControlProvider
{
    Q_OBJECT
public:
    explicit GitCliProvider(QObject *parent = nullptr);
    ~GitCliProvider() override;

    void setWorkspace(const QString &path) override;
    [[nodiscard]] QString workspace() const override { return m_workspace; }
    void refresh() override;
    void stage(const QStringList &paths) override;
    void unstage(const QStringList &paths) override;
    void discard(const QStringList &paths) override;
    void commit(const QString &message) override;
    void requestDiff(const QString &path, bool staged) override;

    [[nodiscard]] ScmStatus lastStatus() const { return m_status; }

private:
    enum class Kind {
        Refresh,
        Mutate,
        Diff,
    };

    struct Command {
        Kind kind = Kind::Refresh;
        QStringList arguments;
        QString diffPath;
        bool refreshAfter = false;
        bool untrackedDiff = false;
    };

    void enqueue(Command command);
    void startNext();
    [[nodiscard]] bool isUntracked(const QString &path) const;

    QString m_workspace;
    ScmStatus m_status;
    QQueue<Command> m_queue;
    Command m_current;
    quint64 m_generation = 0;
    bool m_running = false;
};

#endif
