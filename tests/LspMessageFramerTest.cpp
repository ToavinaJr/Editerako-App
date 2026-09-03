#include "lsp/LspMessageFramer.h"

#include <QtTest>

class LspMessageFramerTest : public QObject
{
    Q_OBJECT

private slots:
    void framesContentLength();
    void splitsPartialThenComplete();
    void twoMessagesInOneChunk();
    void acceptsLfSeparators();
};

void LspMessageFramerTest::framesContentLength()
{
    const QByteArray body = QByteArrayLiteral("{\"jsonrpc\":\"2.0\"}");
    const QByteArray framed = LspMessageFramer::frame(body);
    QVERIFY(framed.startsWith("Content-Length: "));
    QVERIFY(framed.contains("\r\n\r\n"));
    QCOMPARE(framed.mid(framed.indexOf("\r\n\r\n") + 4), body);
}

void LspMessageFramerTest::splitsPartialThenComplete()
{
    const QByteArray body = QByteArrayLiteral("{\"id\":1}");
    const QByteArray framed = LspMessageFramer::frame(body);

    LspMessageFramer framer;
    framer.append(framed.left(12));
    QByteArray out;
    QVERIFY(!framer.takeMessage(&out));

    framer.append(framed.mid(12));
    QVERIFY(framer.takeMessage(&out));
    QCOMPARE(out, body);
    QVERIFY(!framer.takeMessage(&out));
}

void LspMessageFramerTest::twoMessagesInOneChunk()
{
    const QByteArray a = QByteArrayLiteral("{\"a\":1}");
    const QByteArray b = QByteArrayLiteral("{\"b\":2}");
    LspMessageFramer framer;
    framer.append(LspMessageFramer::frame(a) + LspMessageFramer::frame(b));

    QByteArray out;
    QVERIFY(framer.takeMessage(&out));
    QCOMPARE(out, a);
    QVERIFY(framer.takeMessage(&out));
    QCOMPARE(out, b);
}

void LspMessageFramerTest::acceptsLfSeparators()
{
    LspMessageFramer framer;
    framer.append(QByteArrayLiteral("Content-Length: 7\n\n{\"x\":1}"));
    QByteArray out;
    QVERIFY(framer.takeMessage(&out));
    QCOMPARE(out, QByteArrayLiteral("{\"x\":1}"));
}

QTEST_GUILESS_MAIN(LspMessageFramerTest)
#include "LspMessageFramerTest.moc"
