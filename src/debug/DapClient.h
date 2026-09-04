#ifndef EDITERAKO_DAPCLIENT_H
#define EDITERAKO_DAPCLIENT_H

#include "debug/DapTypes.h"

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <functional>

class JsonRpcTransport;

class DapClient : public QObject
{
    Q_OBJECT

public:
    using ResponseCallback = std::function<void(const QJsonObject &body, const QJsonObject &error)>;

    explicit DapClient(JsonRpcTransport *transport, QObject *parent = nullptr);

    [[nodiscard]] JsonRpcTransport *transport() const { return m_transport; }
    [[nodiscard]] bool supportsConfigurationDone() const { return m_supportsConfigurationDone; }

    int sendRequest(const QString &command, const QJsonObject &arguments,
                    const ResponseCallback &callback);

    void initialize(const QString &adapterId, const ResponseCallback &callback);
    void launch(const QJsonObject &arguments, const ResponseCallback &callback);
    void attach(const QJsonObject &arguments, const ResponseCallback &callback);
    void configurationDone(const ResponseCallback &callback);
    void setBreakpoints(const QString &path, const QList<int> &lines0,
                        const ResponseCallback &callback);
    void continueDebug(int threadId, const ResponseCallback &callback);
    void pause(int threadId, const ResponseCallback &callback);
    void next(int threadId, const ResponseCallback &callback);
    void stepIn(int threadId, const ResponseCallback &callback);
    void stepOut(int threadId, const ResponseCallback &callback);
    void stackTrace(int threadId, const ResponseCallback &callback);
    void scopes(int frameId, const ResponseCallback &callback);
    void variables(int variablesReference, const ResponseCallback &callback);
    void evaluate(const QString &expression, int frameId, const ResponseCallback &callback);
    void disconnectSession(bool terminateDebuggee, const ResponseCallback &callback);

signals:
    void initializedEvent();
    void stopped(const DapStoppedEvent &event);
    void continued();
    void terminated();
    void exited(int exitCode);
    void output(const QString &category, const QString &text);
    void protocolError(const QString &message);

private:
    void onMessage(const QJsonObject &message);
    void sendObject(const QJsonObject &object);
    void replyToRequest(int requestSeq, const QString &command, bool success,
                        const QString &message = {});
    void handleEvent(const QString &event, const QJsonObject &body);

    JsonRpcTransport *m_transport = nullptr;
    int m_nextSeq = 0;
    bool m_supportsConfigurationDone = false;
    QHash<int, ResponseCallback> m_pending;
};

#endif
