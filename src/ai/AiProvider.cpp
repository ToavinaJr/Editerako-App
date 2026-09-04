#include "ai/AiProvider.h"

#include "ai/AiCatalog.h"
#include "core/Logging.h"

#include "ai/AnthropicProvider.h"
#include "ai/GeminiProvider.h"
#include "ai/OpenAiCompatProvider.h"

AiProvider::AiProvider(QObject *parent)
    : QObject(parent)
{
}

void AiProvider::cancel()
{
}

bool AiProvider::isBusy() const
{
    return false;
}

AiProvider *AiProvider::create(QObject *parent)
{
    return create(QString(), parent);
}

AiProvider *AiProvider::create(const QString &providerId, QObject *parent)
{
    const AiService service = aiServiceById(providerId);
    if (service.kind == AiService::Kind::Account) {
        return nullptr;
    }
    if (service.id == QLatin1String("anthropic")) {
        return new AnthropicProvider(parent);
    }
    if (service.id == QLatin1String("gemini")) {
        return new GeminiProvider(parent);
    }
    if (service.id == QLatin1String("openai") || service.id == QLatin1String("ollama")) {
        return new OpenAiCompatProvider(service.id, parent);
    }
    qCWarning(lcAi) << "Unknown API provider" << service.id;
    return nullptr;
}
