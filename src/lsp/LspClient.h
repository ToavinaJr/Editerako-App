#ifndef EDITERAKO_LSPCLIENT_H
#define EDITERAKO_LSPCLIENT_H

#include <QHash>
#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QString>
#include <functional>

class JsonRpcTransport;

class LspClient : public QObject
{
    Q_OBJECT

public:
    using ResponseCallback = std::function<void(const QJsonValue &result, const QJsonObject &error)>;

    explicit LspClient(JsonRpcTransport *transport, QObject *parent = nullptr);

    [[nodiscard]] JsonRpcTransport *transport() const { return m_transport; }
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    int sendRequest(const QString &method, const QJsonValue &params, const ResponseCallback &callback);
    void sendNotification(const QString &method, const QJsonValue &params);

    void initialize(const QString &rootUri, const ResponseCallback &callback);
    void shutdown(const ResponseCallback &callback);
    void exit();

signals:
    void notificationReceived(const QString &method, const QJsonValue &params);
    void serverRequestReceived(const QString &method, const QJsonValue &id, const QJsonValue &params);
    void protocolError(const QString &message);

private:
    void onMessage(const QJsonObject &message);
    void sendObject(const QJsonObject &object);
    void replyNull(const QJsonValue &id);

    JsonRpcTransport *m_transport = nullptr;
    int m_nextId = 0;
    bool m_initialized = false;
    QHash<int, ResponseCallback> m_pending;
};

#endif
