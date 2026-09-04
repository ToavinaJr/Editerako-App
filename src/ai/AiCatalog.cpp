#include "ai/AiCatalog.h"

QVector<AiService> aiServices()
{
    return {
        {QStringLiteral("chatgpt"), QStringLiteral("ChatGPT"), AiService::Kind::Account,
         QUrl(QStringLiteral("https://chatgpt.com/")), {}, {}, {}},
        {QStringLiteral("claude"), QStringLiteral("Claude"), AiService::Kind::Account,
         QUrl(QStringLiteral("https://claude.ai/")), {}, {}, {}},
        {QStringLiteral("google"), QStringLiteral("Gemini (Google account)"), AiService::Kind::Account,
         QUrl(QStringLiteral("https://gemini.google.com/app")), {}, {}, {}},
        {QStringLiteral("copilot"), QStringLiteral("Microsoft Copilot"), AiService::Kind::Account,
         QUrl(QStringLiteral("https://copilot.microsoft.com/")), {}, {}, {}},
        {QStringLiteral("openai"), QStringLiteral("OpenAI API"), AiService::Kind::Api, {},
         QStringLiteral("gpt-4o-mini"),
         QStringLiteral("https://api.openai.com/v1/chat/completions"),
         QStringLiteral("OPENAI_API_KEY")},
        {QStringLiteral("anthropic"), QStringLiteral("Anthropic API"), AiService::Kind::Api, {},
         QStringLiteral("claude-3-5-haiku-latest"),
         QStringLiteral("https://api.anthropic.com/v1/messages"),
         QStringLiteral("ANTHROPIC_API_KEY")},
        {QStringLiteral("gemini"), QStringLiteral("Gemini API"), AiService::Kind::Api, {},
         QStringLiteral("gemini-2.0-flash-001"),
         QStringLiteral("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent"),
         QStringLiteral("GEMINI_API_KEY")},
        {QStringLiteral("ollama"), QStringLiteral("Ollama (local)"), AiService::Kind::Api, {},
         QStringLiteral("llama3.2"),
         QStringLiteral("http://127.0.0.1:11434/v1/chat/completions"), {}},
    };
}

AiService aiServiceById(const QString &id)
{
    const QString wanted = id.isEmpty() ? defaultAiProviderId() : id;
    for (const AiService &service : aiServices()) {
        if (service.id == wanted) {
            return service;
        }
    }
    if (wanted != defaultAiProviderId()) {
        return aiServiceById(defaultAiProviderId());
    }
    return aiServices().front();
}

QString defaultAiProviderId()
{
    return QStringLiteral("chatgpt");
}

bool isAccountAiProvider(const QString &id)
{
    return aiServiceById(id).kind == AiService::Kind::Account;
}
