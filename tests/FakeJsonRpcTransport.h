#ifndef EDITERAKO_FAKEJSONRPCTRANSPORT_H
#define EDITERAKO_FAKEJSONRPCTRANSPORT_H

#include "lsp/JsonRpcTransport.h"

#include <QList>

class FakeJsonRpcTransport : public JsonRpcTransport
{
public:
    explicit FakeJsonRpcTransport(QObject *parent = nullptr)
        : JsonRpcTransport(parent)
    {
    }

    QList<QJsonObject> sent;

    void send(const QJsonObject &message) override { sent.append(message); }

    void inject(const QJsonObject &message) { emit messageReceived(message); }
};

#endif
