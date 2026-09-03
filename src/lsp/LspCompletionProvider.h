#ifndef EDITERAKO_LSPCOMPLETIONPROVIDER_H
#define EDITERAKO_LSPCOMPLETIONPROVIDER_H

#include "lsp/LspTypes.h"

#include <QString>
#include <functional>

class LspClient;

class LspCompletionProvider
{
public:
    explicit LspCompletionProvider(LspClient *client);

    using Callback = std::function<void(const QVector<LspCompletionItem> &items)>;
    void complete(const QString &uri, int line, int character, const Callback &callback);

private:
    LspClient *m_client = nullptr;
};

#endif
