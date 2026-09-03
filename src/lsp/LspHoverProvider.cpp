#include "lsp/LspHoverProvider.h"

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

LspHoverProvider::LspHoverProvider(LspClient *client)
    : m_client(client)
{
}

void LspHoverProvider::hover(const QString &uri, int line, int character, const Callback &callback)
{
    if (!m_client) {
        if (callback) {
            callback({});
        }
        return;
    }
    m_client->sendRequest(QStringLiteral("textDocument/hover"),
                          textDocumentPosition(uri, line, character),
                          [callback](const QJsonValue &result, const QJsonObject &) {
                              LspHover hover;
                              if (result.isObject()) {
                                  hover = lspHoverFromJson(result.toObject());
                              }
                              if (callback) {
                                  callback(hover);
                              }
                          });
}

void LspHoverProvider::signatureHelp(const QString &uri, int line, int character,
                                     const SignatureCallback &callback)
{
    if (!m_client) {
        if (callback) {
            callback({});
        }
        return;
    }
    m_client->sendRequest(QStringLiteral("textDocument/signatureHelp"),
                          textDocumentPosition(uri, line, character),
                          [callback](const QJsonValue &result, const QJsonObject &) {
                              if (callback) {
                                  callback(lspSignatureHelpFromJson(result));
                              }
                          });
}
