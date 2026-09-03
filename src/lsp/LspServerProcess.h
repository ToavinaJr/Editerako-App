#ifndef EDITERAKO_LSPSERVERPROCESS_H
#define EDITERAKO_LSPSERVERPROCESS_H

#include <QObject>
#include <QString>
#include <QStringList>

class JsonRpcTransport;
class ProcessJsonRpcTransport;
class QProcess;

class LspServerProcess : public QObject
{
    Q_OBJECT

public:
    explicit LspServerProcess(QObject *parent = nullptr);
    ~LspServerProcess() override;

    [[nodiscard]] bool start(const QString &command, const QStringList &args,
                             const QString &workingDirectory);
    void stop();
    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] JsonRpcTransport *transport() const;

signals:
    void started();
    void failed(const QString &message);
    void finished();

private:
    QProcess *m_process = nullptr;
    ProcessJsonRpcTransport *m_transport = nullptr;
};

#endif
