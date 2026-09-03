#ifndef EDITERAKO_LSPHOVERPROVIDER_H
#define EDITERAKO_LSPHOVERPROVIDER_H

#include "lsp/LspTypes.h"

#include <QString>
#include <functional>

class LspClient;

class LspHoverProvider
{
public:
    explicit LspHoverProvider(LspClient *client);

    using Callback = std::function<void(const LspHover &hover)>;
    void hover(const QString &uri, int line, int character, const Callback &callback);

private:
    LspClient *m_client = nullptr;
};

#endif
