#ifndef EDITERAKO_LSPSERVERMANAGER_H
#define EDITERAKO_LSPSERVERMANAGER_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

class JsonRpcTransport;
class LspClient;
class LspCompletionProvider;
class LspDiagnosticsProvider;
class LspDocumentSync;
class LspHoverProvider;
class LspNavigationProvider;
class LspServerProcess;
class LspSymbolProvider;

struct LspServerSpec {
    QString id;
    QString command;
    QStringList args;
    QStringList languageIds;
};

class LspServerManager : public QObject
{
    Q_OBJECT

public:
    explicit LspServerManager(QObject *parent = nullptr);
    ~LspServerManager() override;

    void registerSpec(const LspServerSpec &spec);
    [[nodiscard]] bool startSpec(const QString &specId, const QString &rootUri,
                                 const QString &workingDirectory = {});
    void stopAll();

    [[nodiscard]] LspClient *clientForLanguage(const QString &languageId) const;
    [[nodiscard]] LspDocumentSync *documentSyncForLanguage(const QString &languageId) const;
    [[nodiscard]] LspDiagnosticsProvider *diagnosticsForLanguage(const QString &languageId) const;
    [[nodiscard]] LspCompletionProvider *completionForLanguage(const QString &languageId) const;
    [[nodiscard]] LspHoverProvider *hoverForLanguage(const QString &languageId) const;
    [[nodiscard]] LspNavigationProvider *navigationForLanguage(const QString &languageId) const;
    [[nodiscard]] LspSymbolProvider *symbolsForLanguage(const QString &languageId) const;

    void attachTransport(const QString &specId, const QStringList &languageIds,
                         JsonRpcTransport *transport);

signals:
    void serverFailed(const QString &specId, const QString &message);

private:
    struct Instance {
        LspServerProcess *process = nullptr;
        LspClient *client = nullptr;
        LspDocumentSync *sync = nullptr;
        LspDiagnosticsProvider *diagnostics = nullptr;
        LspCompletionProvider *completion = nullptr;
        LspHoverProvider *hover = nullptr;
        LspNavigationProvider *navigation = nullptr;
        LspSymbolProvider *symbols = nullptr;
        QString specId;
    };

    Instance *createInstance(const QString &specId, const QStringList &languageIds,
                             JsonRpcTransport *transport, LspServerProcess *process);
    [[nodiscard]] Instance *instanceForLanguage(const QString &languageId) const;
    void destroyInstance(Instance *instance);

    QHash<QString, LspServerSpec> m_specs;
    QHash<QString, Instance *> m_bySpec;
    QHash<QString, Instance *> m_byLanguage;
};

#endif
