#include "lsp/LspDiagnosticsProvider.h"

#include "lsp/LspClient.h"

#include <QJsonArray>
#include <QJsonObject>

LspDiagnosticsProvider::LspDiagnosticsProvider(LspClient *client, QObject *parent)
    : QObject(parent)
{
    if (!client) {
        return;
    }
    connect(client, &LspClient::notificationReceived, this, &LspDiagnosticsProvider::onNotification);
}

void LspDiagnosticsProvider::onNotification(const QString &method, const QJsonValue &params)
{
    if (method != QLatin1String("textDocument/publishDiagnostics") || !params.isObject()) {
        return;
    }
    const QJsonObject obj = params.toObject();
    const QString uri = obj.value(QStringLiteral("uri")).toString();
    QVector<LspDiagnostic> diags;
    const QJsonArray arr = obj.value(QStringLiteral("diagnostics")).toArray();
    for (const QJsonValue &item : arr) {
        diags.append(lspDiagnosticFromJson(item.toObject()));
    }
    emit diagnosticsPublished(uri, diags);
}
