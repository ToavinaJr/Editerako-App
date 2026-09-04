#ifndef EDITERAKO_TASKRUNNER_H
#define EDITERAKO_TASKRUNNER_H

#include "tasks/TaskDefinition.h"

#include <QObject>
#include <QString>

class QProcess;

class TaskRunner : public QObject
{
    Q_OBJECT

public:
    explicit TaskRunner(QObject *parent = nullptr);
    ~TaskRunner() override;

    [[nodiscard]] bool isRunning() const;
    void start(const ProcessSpec &spec);
    void cancel();

signals:
    void started(const QString &title);
    void outputReceived(const QString &chunk);
    void finished(int exitCode, const QString &output);
    void failed(const QString &error);

private:
    void emitFinished(int exitCode);

    QProcess *m_process = nullptr;
    QString m_output;
    bool m_running = false;
};

#endif
