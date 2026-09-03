#include "lsp/LspCompletionProvider.h"

#include "lsp/LspClient.h"

#include <QJsonObject>

namespace {

QJsonObject textDocumentPosition(const QString &uri, int line, int character)
{
    QJsonObject pos{{QStringLiteral("line"), line}, {QStringLiteral("character"), character}};
    QJsonObject doc{{QStringLiteral("uri"), uri}};
    return QJsonObject{{QStringLiteral("textDocument"), doc}, {QStringLiteral("position"), pos}};
}

} // namespace

LspCompletionProvider::LspCompletionProvider(LspClient *client)
    : m_client(client)
{
}

void LspCompletionProvider::complete(const QString &uri, int line, int character,
                                     const Callback &callback)
{
    if (!m_client) {
        if (callback) {
            callback({});
        }
        return;
    }
    m_client->sendRequest(QStringLiteral("textDocument/completion"),
                          textDocumentPosition(uri, line, character),
                          [callback](const QJsonValue &result, const QJsonObject &) {
                              if (callback) {
                                  callback(lspCompletionItemsFromJson(result));
                              }
                          });
}
