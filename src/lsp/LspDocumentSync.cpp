#include "lsp/LspDocumentSync.h"

#include "lsp/LspClient.h"

#include <QJsonArray>
#include <QJsonObject>

LspDocumentSync::LspDocumentSync(LspClient *client)
    : m_client(client)
{
}

void LspDocumentSync::didOpen(const QString &uri, const QString &languageId, int version,
                              const QString &text)
{
    if (!m_client) {
        return;
    }
    QJsonObject doc;
    doc.insert(QStringLiteral("uri"), uri);
    doc.insert(QStringLiteral("languageId"), languageId);
    doc.insert(QStringLiteral("version"), version);
    doc.insert(QStringLiteral("text"), text);
    m_client->sendNotification(QStringLiteral("textDocument/didOpen"),
                               QJsonObject{{QStringLiteral("textDocument"), doc}});
}

void LspDocumentSync::didChange(const QString &uri, int version, const QString &text)
{
    if (!m_client) {
        return;
    }
    QJsonObject doc{{QStringLiteral("uri"), uri}, {QStringLiteral("version"), version}};
    QJsonArray changes{QJsonObject{{QStringLiteral("text"), text}}};
    QJsonObject params;
    params.insert(QStringLiteral("textDocument"), doc);
    params.insert(QStringLiteral("contentChanges"), changes);
    m_client->sendNotification(QStringLiteral("textDocument/didChange"), params);
}

void LspDocumentSync::didSave(const QString &uri)
{
    if (!m_client) {
        return;
    }
    m_client->sendNotification(
        QStringLiteral("textDocument/didSave"),
        QJsonObject{{QStringLiteral("textDocument"), QJsonObject{{QStringLiteral("uri"), uri}}}});
}

void LspDocumentSync::didClose(const QString &uri)
{
    if (!m_client) {
        return;
    }
    m_client->sendNotification(
        QStringLiteral("textDocument/didClose"),
        QJsonObject{{QStringLiteral("textDocument"), QJsonObject{{QStringLiteral("uri"), uri}}}});
}
