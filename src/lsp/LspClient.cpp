#include "lsp/LspClient.h"

#include "core/Logging.h"
#include "lsp/JsonRpcTransport.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>

namespace {

QJsonObject requestObject(int id, const QString &method, const QJsonValue &params)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    obj.insert(QStringLiteral("id"), id);
    obj.insert(QStringLiteral("method"), method);
    obj.insert(QStringLiteral("params"), params);
    return obj;
}

QJsonObject notificationObject(const QString &method, const QJsonValue &params)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    obj.insert(QStringLiteral("method"), method);
    obj.insert(QStringLiteral("params"), params);
    return obj;
}

QJsonObject clientCapabilities()
{
    QJsonObject completionItem;
    completionItem.insert(QStringLiteral("documentationFormat"),
                          QJsonArray{QStringLiteral("plaintext"), QStringLiteral("markdown")});
    QJsonObject completion{{QStringLiteral("completionItem"), completionItem}};
    QJsonObject hover{{QStringLiteral("contentFormat"),
                       QJsonArray{QStringLiteral("plaintext"), QStringLiteral("markdown")}}};
    QJsonObject textDocument;
    textDocument.insert(QStringLiteral("synchronization"),
                        QJsonObject{{QStringLiteral("didSave"), true}});
    textDocument.insert(QStringLiteral("completion"), completion);
    textDocument.insert(QStringLiteral("hover"), hover);
    textDocument.insert(QStringLiteral("signatureHelp"), QJsonObject{});
    textDocument.insert(QStringLiteral("definition"), QJsonObject{});
    textDocument.insert(QStringLiteral("references"), QJsonObject{});
    textDocument.insert(QStringLiteral("rename"), QJsonObject{});
    textDocument.insert(QStringLiteral("documentSymbol"), QJsonObject{});
    textDocument.insert(QStringLiteral("publishDiagnostics"), QJsonObject{});
    QJsonObject workspace;
    workspace.insert(QStringLiteral("symbol"), QJsonObject{});
    return QJsonObject{{QStringLiteral("textDocument"), textDocument},
                       {QStringLiteral("workspace"), workspace}};
}

} // namespace

LspClient::LspClient(JsonRpcTransport *transport, QObject *parent)
    : QObject(parent)
    , m_transport(transport)
{
    if (m_transport) {
        connect(m_transport, &JsonRpcTransport::messageReceived, this, &LspClient::onMessage);
        connect(m_transport, &JsonRpcTransport::errorOccurred, this, &LspClient::protocolError);
    }
}

void LspClient::sendObject(const QJsonObject &object)
{
    if (!m_transport) {
        emit protocolError(QStringLiteral("LSP transport is missing"));
        return;
    }
    m_transport->send(object);
}

int LspClient::sendRequest(const QString &method, const QJsonValue &params,
                           const ResponseCallback &callback)
{
    const int id = ++m_nextId;
    if (callback) {
        m_pending.insert(id, callback);
    }
    sendObject(requestObject(id, method, params));
    return id;
}

void LspClient::sendNotification(const QString &method, const QJsonValue &params)
{
    sendObject(notificationObject(method, params));
}

void LspClient::initialize(const QString &rootUri, const ResponseCallback &callback)
{
    QJsonObject params;
    params.insert(QStringLiteral("processId"), QCoreApplication::applicationPid());
    params.insert(QStringLiteral("rootUri"), rootUri);
    params.insert(QStringLiteral("capabilities"), clientCapabilities());
    sendRequest(QStringLiteral("initialize"), params,
                [this, callback](const QJsonValue &result, const QJsonObject &error) {
                    if (error.isEmpty()) {
                        m_initialized = true;
                        sendNotification(QStringLiteral("initialized"), QJsonObject{});
                        emit initializedChanged(true);
                    }
                    if (callback) {
                        callback(result, error);
                    }
                });
}

void LspClient::shutdown(const ResponseCallback &callback)
{
    sendRequest(QStringLiteral("shutdown"), QJsonValue(),
                [this, callback](const QJsonValue &result, const QJsonObject &error) {
                    m_initialized = false;
                    emit initializedChanged(false);
                    if (callback) {
                        callback(result, error);
                    }
                });
}

void LspClient::exit()
{
    sendNotification(QStringLiteral("exit"), QJsonValue());
    if (m_initialized) {
        m_initialized = false;
        emit initializedChanged(false);
    }
}

void LspClient::replyNull(const QJsonValue &id)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    obj.insert(QStringLiteral("id"), id);
    obj.insert(QStringLiteral("result"), QJsonValue());
    sendObject(obj);
}

void LspClient::onMessage(const QJsonObject &message)
{
    const bool hasId = message.contains(QStringLiteral("id"));
    const bool hasMethod = message.contains(QStringLiteral("method"));

    if (hasId && !hasMethod) {
        const int id = message.value(QStringLiteral("id")).toInt();
        const ResponseCallback cb = m_pending.take(id);
        if (!cb) {
            return;
        }
        if (message.contains(QStringLiteral("error"))) {
            cb(QJsonValue(), message.value(QStringLiteral("error")).toObject());
        } else {
            cb(message.value(QStringLiteral("result")), QJsonObject{});
        }
        return;
    }

    if (hasMethod && hasId) {
        const QString method = message.value(QStringLiteral("method")).toString();
        qCInfo(lcLsp) << "LSP server request" << method;
        emit serverRequestReceived(method, message.value(QStringLiteral("id")),
                                   message.value(QStringLiteral("params")));
        replyNull(message.value(QStringLiteral("id")));
        return;
    }

    if (hasMethod) {
        emit notificationReceived(message.value(QStringLiteral("method")).toString(),
                                  message.value(QStringLiteral("params")));
    }
}
