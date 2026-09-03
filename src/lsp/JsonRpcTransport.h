#ifndef EDITERAKO_JSONRPCTRANSPORT_H
#define EDITERAKO_JSONRPCTRANSPORT_H

#include "lsp/LspMessageFramer.h"

#include <QJsonObject>
#include <QObject>

class QIODevice;

class JsonRpcTransport : public QObject
{
    Q_OBJECT

public:
    explicit JsonRpcTransport(QObject *parent = nullptr);

    virtual void send(const QJsonObject &message) = 0;

signals:
    void messageReceived(const QJsonObject &message);
    void errorOccurred(const QString &message);
};

class ProcessJsonRpcTransport : public JsonRpcTransport
{
    Q_OBJECT

public:
    explicit ProcessJsonRpcTransport(QIODevice *device, QObject *parent = nullptr);

    void send(const QJsonObject &message) override;
    void ingest(const QByteArray &bytes);

private:
    void onReadyRead();

    QIODevice *m_device = nullptr;
    LspMessageFramer m_framer;
};

#endif
