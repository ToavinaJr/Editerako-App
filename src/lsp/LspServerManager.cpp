#include "lsp/LspServerManager.h"

#include "core/Logging.h"
#include "lsp/JsonRpcTransport.h"
#include "lsp/LspClient.h"
#include "lsp/LspCompletionProvider.h"
#include "lsp/LspDiagnosticsProvider.h"
#include "lsp/LspDocumentSync.h"
#include "lsp/LspHoverProvider.h"
#include "lsp/LspNavigationProvider.h"
#include "lsp/LspServerProcess.h"
#include "lsp/LspSymbolProvider.h"

#include <QList>

LspServerManager::LspServerManager(QObject *parent)
    : QObject(parent)
{
}

LspServerManager::~LspServerManager()
{
    stopAll();
}

void LspServerManager::registerSpec(const LspServerSpec &spec)
{
    if (!spec.id.isEmpty()) {
        m_specs.insert(spec.id, spec);
    }
}

LspServerManager::Instance *LspServerManager::createInstance(const QString &specId,
                                                             const QStringList &languageIds,
                                                             JsonRpcTransport *transport,
                                                             LspServerProcess *process)
{
    auto *instance = new Instance;
    instance->specId = specId;
    instance->process = process;
    instance->client = new LspClient(transport, this);
    instance->sync = new LspDocumentSync(instance->client);
    instance->diagnostics = new LspDiagnosticsProvider(instance->client, this);
    instance->completion = new LspCompletionProvider(instance->client);
    instance->hover = new LspHoverProvider(instance->client);
    instance->navigation = new LspNavigationProvider(instance->client);
    instance->symbols = new LspSymbolProvider(instance->client);

    m_bySpec.insert(specId, instance);
    for (const QString &languageId : languageIds) {
        m_byLanguage.insert(languageId, instance);
    }
    return instance;
}

void LspServerManager::destroyInstance(Instance *instance)
{
    if (!instance) {
        return;
    }
    m_bySpec.remove(instance->specId);
    for (auto it = m_byLanguage.begin(); it != m_byLanguage.end();) {
        if (it.value() == instance) {
            it = m_byLanguage.erase(it);
        } else {
            ++it;
        }
    }
    delete instance->symbols;
    delete instance->navigation;
    delete instance->hover;
    delete instance->completion;
    delete instance->sync;
    delete instance->diagnostics;
    delete instance->client;
    if (instance->process) {
        instance->process->stop();
        instance->process->deleteLater();
    }
    delete instance;
}

LspServerManager::Instance *LspServerManager::instanceForLanguage(const QString &languageId) const
{
    return m_byLanguage.value(languageId, nullptr);
}

bool LspServerManager::startSpec(const QString &specId, const QString &rootUri,
                                 const QString &workingDirectory)
{
    const LspServerSpec spec = m_specs.value(specId);
    if (spec.id.isEmpty() || spec.command.isEmpty()) {
        const QString message = QStringLiteral("Unknown or incomplete LSP spec");
        qCWarning(lcLsp) << message << specId;
        emit serverFailed(specId, message);
        return false;
    }
    if (m_bySpec.contains(specId)) {
        destroyInstance(m_bySpec.value(specId));
    }

    auto *process = new LspServerProcess(this);
    connect(process, &LspServerProcess::failed, this, [this, specId](const QString &message) {
        emit serverFailed(specId, message);
    });
    if (!process->start(spec.command, spec.args, workingDirectory)) {
        process->deleteLater();
        return false;
    }

    Instance *instance = createInstance(specId, spec.languageIds, process->transport(), process);
    connect(process, &LspServerProcess::started, this, [this, specId, rootUri]() {
        Instance *ready = m_bySpec.value(specId, nullptr);
        if (ready && ready->client && !ready->client->isInitialized()) {
            ready->client->initialize(rootUri, {});
        }
    });
    if (process->isRunning()) {
        instance->client->initialize(rootUri, {});
    }
    return true;
}

bool LspServerManager::ensureSpec(const QString &specId, const QString &rootUri,
                                  const QString &workingDirectory)
{
    if (m_bySpec.contains(specId)) {
        return true;
    }
    return startSpec(specId, rootUri, workingDirectory);
}

void LspServerManager::attachTransport(const QString &specId, const QStringList &languageIds,
                                       JsonRpcTransport *transport)
{
    if (m_bySpec.contains(specId)) {
        destroyInstance(m_bySpec.value(specId));
    }
    createInstance(specId, languageIds, transport, nullptr);
}

void LspServerManager::stopAll()
{
    const QList<Instance *> instances = m_bySpec.values();
    for (Instance *instance : instances) {
        destroyInstance(instance);
    }
}

LspClient *LspServerManager::clientForLanguage(const QString &languageId) const
{
    Instance *instance = instanceForLanguage(languageId);
    return instance ? instance->client : nullptr;
}

LspDocumentSync *LspServerManager::documentSyncForLanguage(const QString &languageId) const
{
    Instance *instance = instanceForLanguage(languageId);
    return instance ? instance->sync : nullptr;
}

LspDiagnosticsProvider *LspServerManager::diagnosticsForLanguage(const QString &languageId) const
{
    Instance *instance = instanceForLanguage(languageId);
    return instance ? instance->diagnostics : nullptr;
}

LspCompletionProvider *LspServerManager::completionForLanguage(const QString &languageId) const
{
    Instance *instance = instanceForLanguage(languageId);
    return instance ? instance->completion : nullptr;
}

LspHoverProvider *LspServerManager::hoverForLanguage(const QString &languageId) const
{
    Instance *instance = instanceForLanguage(languageId);
    return instance ? instance->hover : nullptr;
}

LspNavigationProvider *LspServerManager::navigationForLanguage(const QString &languageId) const
{
    Instance *instance = instanceForLanguage(languageId);
    return instance ? instance->navigation : nullptr;
}

LspSymbolProvider *LspServerManager::symbolsForLanguage(const QString &languageId) const
{
    Instance *instance = instanceForLanguage(languageId);
    return instance ? instance->symbols : nullptr;
}
