#ifndef EDITERAKO_GITCLIPROVIDER_H
#define EDITERAKO_GITCLIPROVIDER_H

#include "scm/ISourceControlProvider.h"

#include <QQueue>

class QProcess;

class GitCliProvider final : public ISourceControlProvider
{
    Q_OBJECT
public:
    explicit GitCliProvider(QObject *parent = nullptr);
    void setWorkspace(const QString &path) override;
    [[nodiscard]] QString workspace() const override { return m_workspace; }
    void refresh() override;
    void stage(const QStringList &paths) override;
    void unstage(const QStringList &paths) override;
    void discard(const QStringList &paths) override;
    void commit(const QString &message) override;
    void requestDiff(const QString &path, bool staged) override;

private:
    struct Command { QStringList arguments; QString diffPath; bool refreshAfter = false; };
    void enqueue(Command command);
    void startNext();
    void handleFinished(int exitCode);

    QString m_workspace;
    QProcess *m_process = nullptr;
    QQueue<Command> m_queue;
    Command m_current;
};

#endif

