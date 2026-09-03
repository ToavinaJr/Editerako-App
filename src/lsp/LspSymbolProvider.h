#ifndef EDITERAKO_LSPSYMBOLPROVIDER_H
#define EDITERAKO_LSPSYMBOLPROVIDER_H

#include "lsp/LspTypes.h"

#include <QString>
#include <functional>

class LspClient;

class LspSymbolProvider
{
public:
    explicit LspSymbolProvider(LspClient *client);

    using Callback = std::function<void(const QVector<LspSymbol> &symbols)>;
    void documentSymbols(const QString &uri, const Callback &callback);
    void workspaceSymbols(const QString &query, const Callback &callback);

private:
    LspClient *m_client = nullptr;
};

#endif
