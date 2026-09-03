#ifndef EDITERAKO_LSPNAVIGATIONPROVIDER_H
#define EDITERAKO_LSPNAVIGATIONPROVIDER_H

#include "lsp/LspTypes.h"

#include <QJsonObject>
#include <QString>
#include <functional>

class LspClient;

class LspNavigationProvider
{
public:
    explicit LspNavigationProvider(LspClient *client);

    using Callback = std::function<void(const QVector<LspLocation> &locations)>;
    void definition(const QString &uri, int line, int character, const Callback &callback);
    void references(const QString &uri, int line, int character, const Callback &callback);
    void rename(const QString &uri, int line, int character, const QString &newName,
                const std::function<void(const QJsonObject &edit)> &callback);

private:
    void requestLocations(const QString &method, const QString &uri, int line, int character,
                          const Callback &callback);

    LspClient *m_client = nullptr;
};

#endif
