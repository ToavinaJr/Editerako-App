#include "lsp/JsonRpcTransport.h"

#include "core/Logging.h"

#include <QIODevice>
#include <QJsonDocument>
#include <QJsonParseError>

JsonRpcTransport::JsonRpcTransport(QObject *parent)
    : QObject(parent)
{
}

ProcessJsonRpcTransport::ProcessJsonRpcTransport(QIODevice *device, QObject *parent)
    : JsonRpcTransport(parent)
    , m_device(device)
{
    if (m_device) {
        connect(m_device, &QIODevice::readyRead, this, &ProcessJsonRpcTransport::onReadyRead);
    }
}

void ProcessJsonRpcTransport::send(const QJsonObject &message)
{
    if (!m_device || !m_device->isWritable()) {
        emit errorOccurred(QStringLiteral("LSP transport is not writable"));
        return;
    }
    const QByteArray body = QJsonDocument(message).toJson(QJsonDocument::Compact);
    if (m_device->write(LspMessageFramer::frame(body)) < 0) {
        emit errorOccurred(QStringLiteral("Failed to write LSP message"));
    }
}

void ProcessJsonRpcTransport::ingest(const QByteArray &bytes)
{
    m_framer.append(bytes);
    QByteArray json;
    while (m_framer.takeMessage(&json)) {
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            qCWarning(lcLsp) << "Invalid JSON-RPC payload" << err.errorString();
            emit errorOccurred(QStringLiteral("Invalid JSON-RPC payload"));
            continue;
        }
        emit messageReceived(doc.object());
    }
}

void ProcessJsonRpcTransport::onReadyRead()
{
    if (!m_device) {
        return;
    }
    ingest(m_device->readAll());
}
