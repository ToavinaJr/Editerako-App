#include "ai/AiCatalog.h"

#include <QtTest>

class AiCatalogTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultIsAccountChatNotGeminiApi();
    void knownServicesResolve();
    void unknownFallsBackToDefault();
};

void AiCatalogTest::defaultIsAccountChatNotGeminiApi()
{
    QCOMPARE(defaultAiProviderId(), QStringLiteral("chatgpt"));
    QVERIFY(isAccountAiProvider(defaultAiProviderId()));
    QVERIFY(!isAccountAiProvider(QStringLiteral("gemini")));
    QVERIFY(!isAccountAiProvider(QStringLiteral("openai")));
}

void AiCatalogTest::knownServicesResolve()
{
    QCOMPARE(aiServiceById(QStringLiteral("claude")).kind, AiService::Kind::Account);
    QCOMPARE(aiServiceById(QStringLiteral("openai")).apiKeyEnv, QStringLiteral("OPENAI_API_KEY"));
    QVERIFY(aiServiceById(QStringLiteral("ollama")).apiKeyEnv.isEmpty());
    QVERIFY(aiServiceById(QStringLiteral("chatgpt")).accountUrl.isValid());
}

void AiCatalogTest::unknownFallsBackToDefault()
{
    QCOMPARE(aiServiceById(QStringLiteral("not-a-provider")).id, defaultAiProviderId());
}

QTEST_GUILESS_MAIN(AiCatalogTest)
#include "AiCatalogTest.moc"
