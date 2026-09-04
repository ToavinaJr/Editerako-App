#include "terminal/AnsiSgr.h"

#include <QtTest>

class AnsiSgrTest : public QObject
{
    Q_OBJECT

private slots:
    void stripsColorCodes();
    void mapsForegroundColors();
    void keepsStateAcrossChunks();
    void resetAndDefaultRestoreColor();
};

void AnsiSgrTest::stripsColorCodes()
{
    QCOMPARE(stripAnsi(QStringLiteral("plain")), QStringLiteral("plain"));
    QCOMPARE(stripAnsi(QStringLiteral("\x1b[32mgreen\x1b[0m")), QStringLiteral("green"));
    QCOMPARE(stripAnsi(QStringLiteral("a\x1b[1;31mb\x1b[0mc")), QStringLiteral("abc"));
}

void AnsiSgrTest::mapsForegroundColors()
{
    AnsiSgrDecoder decoder;
    const QVector<AnsiFragment> fragments = decoder.feed(QStringLiteral("\x1b[32mok\x1b[0m"));
    QCOMPARE(fragments.size(), 1);
    QCOMPARE(fragments.at(0).text, QStringLiteral("ok"));
    QCOMPARE(fragments.at(0).color, QColor(152, 195, 121));
    QCOMPARE(decoder.currentColor(), QColor(204, 204, 204));
}

void AnsiSgrTest::keepsStateAcrossChunks()
{
    AnsiSgrDecoder decoder;
    const QVector<AnsiFragment> first = decoder.feed(QStringLiteral("\x1b[31mhe"));
    QCOMPARE(first.size(), 1);
    QCOMPARE(first.front().text, QStringLiteral("he"));
    QCOMPARE(first.front().color, QColor(224, 108, 117));

    const QVector<AnsiFragment> second = decoder.feed(QStringLiteral("llo\x1b[0m"));
    QCOMPARE(second.size(), 1);
    QCOMPARE(second.front().text, QStringLiteral("llo"));
    QCOMPARE(second.front().color, QColor(224, 108, 117));
}

void AnsiSgrTest::resetAndDefaultRestoreColor()
{
    AnsiSgrDecoder decoder;
    const QVector<AnsiFragment> bright = decoder.feed(QStringLiteral("\x1b[94mbright"));
    QVERIFY(!bright.isEmpty());
    QCOMPARE(decoder.currentColor(), QColor(102, 217, 239));
    const QVector<AnsiFragment> restored = decoder.feed(QStringLiteral("\x1b[39m"));
    Q_UNUSED(restored);
    QCOMPARE(decoder.currentColor(), QColor(204, 204, 204));
    decoder.reset();
    QCOMPARE(decoder.currentColor(), QColor(204, 204, 204));
}

QTEST_GUILESS_MAIN(AnsiSgrTest)
#include "AnsiSgrTest.moc"
