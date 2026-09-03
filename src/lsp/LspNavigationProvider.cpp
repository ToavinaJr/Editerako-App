#include "lsp/LspNavigationProvider.h"

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

LspNavigationProvider::LspNavigationProvider(LspClient *client)
    : m_client(client)
{
}

void LspNavigationProvider::requestLocations(const QString &method, const QString &uri, int line,
                                             int character, const Callback &callback)
{
    if (!m_client) {
        if (callback) {
            callback({});
        }
        return;
    }
    m_client->sendRequest(method, textDocumentPosition(uri, line, character),
                          [callback](const QJsonValue &result, const QJsonObject &) {
                              if (callback) {
                                  callback(lspLocationsFromJson(result));
                              }
                          });
}

void LspNavigationProvider::definition(const QString &uri, int line, int character,
                                       const Callback &callback)
{
    requestLocations(QStringLiteral("textDocument/definition"), uri, line, character, callback);
}

void LspNavigationProvider::references(const QString &uri, int line, int character,
                                       const Callback &callback)
{
    if (!m_client) {
        if (callback) {
            callback({});
        }
        return;
    }
    QJsonObject params = textDocumentPosition(uri, line, character);
    params.insert(QStringLiteral("context"), QJsonObject{{QStringLiteral("includeDeclaration"), true}});
    m_client->sendRequest(QStringLiteral("textDocument/references"), params,
                          [callback](const QJsonValue &result, const QJsonObject &) {
                              if (callback) {
                                  callback(lspLocationsFromJson(result));
                              }
                          });
}

void LspNavigationProvider::rename(const QString &uri, int line, int character, const QString &newName,
                                   const std::function<void(const QJsonObject &edit)> &callback)
{
    if (!m_client) {
        if (callback) {
            callback({});
        }
        return;
    }
    QJsonObject params = textDocumentPosition(uri, line, character);
    params.insert(QStringLiteral("newName"), newName);
    m_client->sendRequest(QStringLiteral("textDocument/rename"), params,
                          [callback](const QJsonValue &result, const QJsonObject &) {
                              if (callback) {
                                  callback(result.toObject());
                              }
                          });
}
