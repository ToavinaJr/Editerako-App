#include "lsp/LspSymbolProvider.h"

#include "lsp/LspClient.h"

#include <QJsonObject>

LspSymbolProvider::LspSymbolProvider(LspClient *client)
    : m_client(client)
{
}

void LspSymbolProvider::documentSymbols(const QString &uri, const Callback &callback)
{
    if (!m_client) {
        if (callback) {
            callback({});
        }
        return;
    }
    m_client->sendRequest(QStringLiteral("textDocument/documentSymbol"),
                          QJsonObject{{QStringLiteral("textDocument"),
                                       QJsonObject{{QStringLiteral("uri"), uri}}}},
                          [callback](const QJsonValue &result, const QJsonObject &) {
                              if (callback) {
                                  callback(lspDocumentSymbolsFromJson(result));
                              }
                          });
}

void LspSymbolProvider::workspaceSymbols(const QString &query, const Callback &callback)
{
    if (!m_client) {
        if (callback) {
            callback({});
        }
        return;
    }
    m_client->sendRequest(QStringLiteral("workspace/symbol"),
                          QJsonObject{{QStringLiteral("query"), query}},
                          [callback](const QJsonValue &result, const QJsonObject &) {
                              if (callback) {
                                  callback(lspWorkspaceSymbolsFromJson(result));
                              }
                          });
}
