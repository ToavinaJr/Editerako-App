#ifndef EDITERAKO_LSPDOCUMENTSYNC_H
#define EDITERAKO_LSPDOCUMENTSYNC_H

#include <QString>

class LspClient;

class LspDocumentSync
{
public:
    explicit LspDocumentSync(LspClient *client);

    void didOpen(const QString &uri, const QString &languageId, int version, const QString &text);
    void didChange(const QString &uri, int version, const QString &text);
    void didSave(const QString &uri);
    void didClose(const QString &uri);

private:
    LspClient *m_client = nullptr;
};

#endif
