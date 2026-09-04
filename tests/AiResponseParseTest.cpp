#include "ai/AiResponseParse.h"

#include <QtTest>

class AiResponseParseTest : public QObject
{
    Q_OBJECT

private slots:
    void geminiParts();
    void openAiMessage();
    void anthropicTextBlocks();
    void openAiSseDelta();
};

void AiResponseParseTest::geminiParts()
{
    const QByteArray json = R"({"candidates":[{"content":{"parts":[{"text":"hi "},{"text":"there"}]}}]})";
    QCOMPARE(parseGeminiResponse(json), QStringLiteral("hi there"));
}

void AiResponseParseTest::openAiMessage()
{
    const QByteArray json =
        R"({"choices":[{"message":{"role":"assistant","content":"hello-openai"}}]})";
    QCOMPARE(parseOpenAiChatResponse(json), QStringLiteral("hello-openai"));
}

void AiResponseParseTest::anthropicTextBlocks()
{
    const QByteArray json =
        R"({"content":[{"type":"text","text":"ab"},{"type":"text","text":"cd"}]})";
    QCOMPARE(parseAnthropicResponse(json), QStringLiteral("abcd"));
}

void AiResponseParseTest::openAiSseDelta()
{
    QCOMPARE(parseOpenAiSseDelta("data: {\"choices\":[{\"delta\":{\"content\":\"tok\"}}]}"),
             QStringLiteral("tok"));
    QVERIFY(parseOpenAiSseDelta("data: [DONE]").isEmpty());
}

QTEST_GUILESS_MAIN(AiResponseParseTest)
#include "AiResponseParseTest.moc"
