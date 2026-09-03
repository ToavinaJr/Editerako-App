#include "FakeJsonRpcTransport.h"
#include "lsp/LspClient.h"
#include "lsp/LspCompletionProvider.h"
#include "lsp/LspDiagnosticsProvider.h"
#include "lsp/LspDocumentSync.h"
#include "lsp/LspHoverProvider.h"
#include "lsp/LspNavigationProvider.h"
#include "lsp/LspServerManager.h"
#include "lsp/LspSymbolProvider.h"
#include "lsp/LspTypes.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QtTest>

class LspClientTest : public QObject
{
    Q_OBJECT

private slots:
    void initializeHandshake();
    void requestErrorResponse();
    void didOpenNotification();
    void publishDiagnostics();
    void completionAndHover();
    void definitionAndSymbols();
    void managerAttachTransport();
    void serverRequestGetsNullResult();
};

void LspClientTest::initializeHandshake()
{
    FakeJsonRpcTransport transport;
    LspClient client(&transport);
    bool done = false;
    client.initialize(QStringLiteral("file:///tmp/proj"),
                      [&](const QJsonValue &result, const QJsonObject &error) {
                          QVERIFY(error.isEmpty());
                          QCOMPARE(result.toObject().value(QStringLiteral("serverName")).toString(),
                                   QStringLiteral("mock"));
                          done = true;
                      });

    QCOMPARE(transport.sent.size(), 1);
    QCOMPARE(transport.sent.front().value(QStringLiteral("method")).toString(),
             QStringLiteral("initialize"));
    const int id = transport.sent.front().value(QStringLiteral("id")).toInt();

    transport.inject(QJsonObject{{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                                 {QStringLiteral("id"), id},
                                 {QStringLiteral("result"),
                                  QJsonObject{{QStringLiteral("serverName"), QStringLiteral("mock")}}}});
    QVERIFY(done);
    QVERIFY(client.isInitialized());
    QCOMPARE(transport.sent.size(), 2);
    QCOMPARE(transport.sent.at(1).value(QStringLiteral("method")).toString(),
             QStringLiteral("initialized"));
}

void LspClientTest::requestErrorResponse()
{
    FakeJsonRpcTransport transport;
    LspClient client(&transport);
    bool sawError = false;
    const int id = client.sendRequest(QStringLiteral("textDocument/hover"), QJsonObject{},
                                      [&](const QJsonValue &, const QJsonObject &error) {
                                          sawError = !error.isEmpty();
                                          QCOMPARE(error.value(QStringLiteral("code")).toInt(), -32603);
                                      });
    transport.inject(QJsonObject{{QStringLiteral("id"), id},
                                 {QStringLiteral("error"),
                                  QJsonObject{{QStringLiteral("code"), -32603},
                                              {QStringLiteral("message"), QStringLiteral("fail")}}}});
    QVERIFY(sawError);
}

void LspClientTest::didOpenNotification()
{
    FakeJsonRpcTransport transport;
    LspClient client(&transport);
    LspDocumentSync sync(&client);
    sync.didOpen(QStringLiteral("file:///a.cpp"), QStringLiteral("cpp"), 1, QStringLiteral("int x;"));

    QCOMPARE(transport.sent.size(), 1);
    QCOMPARE(transport.sent.front().value(QStringLiteral("method")).toString(),
             QStringLiteral("textDocument/didOpen"));
    const QJsonObject doc = transport.sent.front()
                                .value(QStringLiteral("params"))
                                .toObject()
                                .value(QStringLiteral("textDocument"))
                                .toObject();
    QCOMPARE(doc.value(QStringLiteral("languageId")).toString(), QStringLiteral("cpp"));
    QCOMPARE(doc.value(QStringLiteral("version")).toInt(), 1);
    QCOMPARE(doc.value(QStringLiteral("text")).toString(), QStringLiteral("int x;"));
}

void LspClientTest::publishDiagnostics()
{
    FakeJsonRpcTransport transport;
    LspClient client(&transport);
    LspDiagnosticsProvider provider(&client);
    QSignalSpy spy(&provider, &LspDiagnosticsProvider::diagnosticsPublished);

    QJsonObject diagnostic;
    diagnostic.insert(QStringLiteral("message"), QStringLiteral("err"));
    diagnostic.insert(QStringLiteral("severity"), 1);
    diagnostic.insert(QStringLiteral("range"),
                      lspRangeToJson(LspRange{LspPosition{0, 0}, LspPosition{0, 1}}));

    QJsonObject params;
    params.insert(QStringLiteral("uri"), QStringLiteral("file:///a.cpp"));
    params.insert(QStringLiteral("diagnostics"), QJsonArray{diagnostic});

    QJsonObject message;
    message.insert(QStringLiteral("method"), QStringLiteral("textDocument/publishDiagnostics"));
    message.insert(QStringLiteral("params"), params);
    transport.inject(message);

    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.front().at(0).toString(), QStringLiteral("file:///a.cpp"));
}

void LspClientTest::completionAndHover()
{
    FakeJsonRpcTransport transport;
    LspClient client(&transport);
    LspCompletionProvider completion(&client);
    LspHoverProvider hover(&client);

    QVector<LspCompletionItem> items;
    completion.complete(QStringLiteral("file:///a.cpp"), 0, 1, [&](const QVector<LspCompletionItem> &v) {
        items = v;
    });
    const int completionId = transport.sent.front().value(QStringLiteral("id")).toInt();
    transport.inject(QJsonObject{
        {QStringLiteral("id"), completionId},
        {QStringLiteral("result"),
         QJsonArray{QJsonObject{{QStringLiteral("label"), QStringLiteral("printf")}}}}});
    QCOMPARE(items.size(), 1);
    QCOMPARE(items.front().label, QStringLiteral("printf"));

    LspHover result;
    hover.hover(QStringLiteral("file:///a.cpp"), 0, 1, [&](const LspHover &h) { result = h; });
    const int hoverId = transport.sent.at(1).value(QStringLiteral("id")).toInt();
    transport.inject(QJsonObject{
        {QStringLiteral("id"), hoverId},
        {QStringLiteral("result"), QJsonObject{{QStringLiteral("contents"), QStringLiteral("int")}}}});
    QCOMPARE(result.contents, QStringLiteral("int"));
}

void LspClientTest::definitionAndSymbols()
{
    FakeJsonRpcTransport transport;
    LspClient client(&transport);
    LspNavigationProvider nav(&client);
    LspSymbolProvider symbols(&client);

    QVector<LspLocation> locs;
    nav.definition(QStringLiteral("file:///a.cpp"), 1, 2, [&](const QVector<LspLocation> &v) { locs = v; });
    const int defId = transport.sent.front().value(QStringLiteral("id")).toInt();
    QJsonObject location;
    location.insert(QStringLiteral("uri"), QStringLiteral("file:///b.cpp"));
    location.insert(QStringLiteral("range"),
                    lspRangeToJson(LspRange{LspPosition{4, 0}, LspPosition{4, 1}}));
    transport.inject(QJsonObject{{QStringLiteral("id"), defId}, {QStringLiteral("result"), location}});
    QCOMPARE(locs.size(), 1);
    QCOMPARE(locs.front().uri, QStringLiteral("file:///b.cpp"));

    QVector<LspSymbol> syms;
    symbols.documentSymbols(QStringLiteral("file:///a.cpp"), [&](const QVector<LspSymbol> &v) { syms = v; });
    const int symId = transport.sent.at(1).value(QStringLiteral("id")).toInt();
    QJsonObject symbol;
    symbol.insert(QStringLiteral("name"), QStringLiteral("main"));
    symbol.insert(QStringLiteral("kind"), 12);
    symbol.insert(QStringLiteral("range"),
                  lspRangeToJson(LspRange{LspPosition{0, 0}, LspPosition{1, 0}}));
    transport.inject(QJsonObject{{QStringLiteral("id"), symId}, {QStringLiteral("result"), QJsonArray{symbol}}});
    QCOMPARE(syms.size(), 1);
    QCOMPARE(syms.front().name, QStringLiteral("main"));
}

void LspClientTest::managerAttachTransport()
{
    FakeJsonRpcTransport transport;
    LspServerManager manager;
    manager.attachTransport(QStringLiteral("mock"), {QStringLiteral("cpp")}, &transport);
    QVERIFY(manager.clientForLanguage(QStringLiteral("cpp")) != nullptr);
    QVERIFY(manager.documentSyncForLanguage(QStringLiteral("cpp")) != nullptr);
    QVERIFY(manager.clientForLanguage(QStringLiteral("python")) == nullptr);

    manager.documentSyncForLanguage(QStringLiteral("cpp"))
        ->didSave(QStringLiteral("file:///a.cpp"));
    QCOMPARE(transport.sent.front().value(QStringLiteral("method")).toString(),
             QStringLiteral("textDocument/didSave"));
}

void LspClientTest::serverRequestGetsNullResult()
{
    FakeJsonRpcTransport transport;
    LspClient client(&transport);
    transport.inject(QJsonObject{{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                                 {QStringLiteral("id"), 9},
                                 {QStringLiteral("method"), QStringLiteral("client/registerCapability")},
                                 {QStringLiteral("params"), QJsonObject{}}});
    QCOMPARE(transport.sent.size(), 1);
    QCOMPARE(transport.sent.front().value(QStringLiteral("id")).toInt(), 9);
    QVERIFY(transport.sent.front().contains(QStringLiteral("result")));
}

QTEST_GUILESS_MAIN(LspClientTest)
#include "LspClientTest.moc"
