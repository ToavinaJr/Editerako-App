#include "lsp/LspTypes.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QtTest>

class LspTypesTest : public QObject
{
    Q_OBJECT

private slots:
    void positionRoundTrip();
    void diagnosticFromJson();
    void hoverMarkupAndString();
    void completionItemsListAndObject();
    void locationsArrayAndLink();
    void fileUriRoundTrip();
};

void LspTypesTest::positionRoundTrip()
{
    LspPosition pos{3, 8};
    const LspPosition back = lspPositionFromJson(lspPositionToJson(pos));
    QCOMPARE(back.line, 3);
    QCOMPARE(back.character, 8);
}

void LspTypesTest::diagnosticFromJson()
{
    const QJsonObject json{{QStringLiteral("range"),
                            lspRangeToJson(LspRange{LspPosition{1, 0}, LspPosition{1, 4}})},
                           {QStringLiteral("severity"), 2},
                           {QStringLiteral("message"), QStringLiteral("unused")},
                           {QStringLiteral("source"), QStringLiteral("clang")},
                           {QStringLiteral("code"), 123}};
    const LspDiagnostic diag = lspDiagnosticFromJson(json);
    QCOMPARE(diag.severity, LspSeverity::Warning);
    QCOMPARE(diag.message, QStringLiteral("unused"));
    QCOMPARE(diag.code, QStringLiteral("123"));
    QCOMPARE(diag.range.start.line, 1);
}

void LspTypesTest::hoverMarkupAndString()
{
    QCOMPARE(lspMarkupToString(QJsonValue(QStringLiteral("plain"))), QStringLiteral("plain"));
    const QJsonObject markup{{QStringLiteral("kind"), QStringLiteral("markdown")},
                             {QStringLiteral("value"), QStringLiteral("**T**")}};
    QCOMPARE(lspMarkupToString(markup), QStringLiteral("**T**"));
    const LspHover hover = lspHoverFromJson(QJsonObject{{QStringLiteral("contents"), markup}});
    QCOMPARE(hover.contents, QStringLiteral("**T**"));
}

void LspTypesTest::completionItemsListAndObject()
{
    const QJsonArray arr{QJsonObject{{QStringLiteral("label"), QStringLiteral("foo")},
                                     {QStringLiteral("kind"), 3}}};
    QCOMPARE(lspCompletionItemsFromJson(arr).size(), 1);
    QCOMPARE(lspCompletionItemsFromJson(arr).front().label, QStringLiteral("foo"));

    const QJsonObject wrapped{{QStringLiteral("isIncomplete"), false}, {QStringLiteral("items"), arr}};
    QCOMPARE(lspCompletionItemsFromJson(wrapped).size(), 1);
}

void LspTypesTest::locationsArrayAndLink()
{
    const QJsonObject loc{{QStringLiteral("uri"), QStringLiteral("file:///a.cpp")},
                          {QStringLiteral("range"),
                           lspRangeToJson(LspRange{LspPosition{2, 0}, LspPosition{2, 1}})}};
    QCOMPARE(lspLocationsFromJson(loc).size(), 1);
    QCOMPARE(lspLocationsFromJson(QJsonArray{loc}).size(), 1);

    const QJsonObject link{{QStringLiteral("targetUri"), QStringLiteral("file:///b.cpp")},
                           {QStringLiteral("targetRange"),
                            lspRangeToJson(LspRange{LspPosition{4, 0}, LspPosition{4, 3}})},
                           {QStringLiteral("targetSelectionRange"),
                            lspRangeToJson(LspRange{LspPosition{4, 1}, LspPosition{4, 2}})}};
    const QVector<LspLocation> links = lspLocationsFromJson(link);
    QCOMPARE(links.size(), 1);
    QCOMPARE(links.front().uri, QStringLiteral("file:///b.cpp"));
    QCOMPARE(links.front().range.start.character, 1);
}

void LspTypesTest::fileUriRoundTrip()
{
    const QString path = QStringLiteral("C:/tmp/hello.cpp");
    const QString uri = lspFileUri(path);
    QVERIFY(uri.startsWith(QStringLiteral("file:")));
    QVERIFY(lspUriToPath(uri).endsWith(QStringLiteral("hello.cpp")));
}

QTEST_GUILESS_MAIN(LspTypesTest)
#include "LspTypesTest.moc"
