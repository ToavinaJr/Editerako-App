#include "core/TextFileFormat.h"

#include <QtTest>

class TextFileFormatTest : public QObject
{
    Q_OBJECT

private slots:
    void detectLfAndCrLf();
    void utf8Roundtrip();
    void utf8BomRoundtrip();
    void crlfBytesPreserved();
    void latin1Fallback();
    void latin1UpgradesWhenUnicode();
    void utf16LeRoundtrip();
    void invalidUtf8IsLatin1();
    void emptyFileMeta();
};

void TextFileFormatTest::detectLfAndCrLf()
{
    QCOMPARE(static_cast<int>(detectLineEnding(QStringLiteral("a\nb\n"))),
             static_cast<int>(LineEnding::Lf));
    QCOMPARE(static_cast<int>(detectLineEnding(QStringLiteral("a\r\nb\r\n"))),
             static_cast<int>(LineEnding::CrLf));
    QCOMPARE(static_cast<int>(detectLineEnding(QStringLiteral("a\rb\r"))),
             static_cast<int>(LineEnding::Cr));
}

void TextFileFormatTest::utf8Roundtrip()
{
    TextFileMeta meta;
    meta.encoding = TextEncoding::Utf8;
    meta.lineEnding = LineEnding::Lf;
    meta.bom = false;
    const QString text = QStringLiteral("café 你好\nline2");
    const EncodedText encoded = encodeText(text, meta);
    const DecodedText decoded = decodeBytes(encoded.bytes);
    QCOMPARE(decoded.text, text);
    QCOMPARE(static_cast<int>(decoded.meta.encoding), static_cast<int>(TextEncoding::Utf8));
    QVERIFY(!decoded.meta.bom);
    QCOMPARE(static_cast<int>(decoded.meta.lineEnding), static_cast<int>(LineEnding::Lf));
}

void TextFileFormatTest::utf8BomRoundtrip()
{
    TextFileMeta meta;
    meta.encoding = TextEncoding::Utf8;
    meta.lineEnding = LineEnding::Lf;
    meta.bom = true;
    const EncodedText encoded = encodeText(QStringLiteral("abc"), meta);
    QVERIFY(encoded.bytes.startsWith("\xEF\xBB\xBF"));
    const DecodedText decoded = decodeBytes(encoded.bytes);
    QCOMPARE(decoded.text, QStringLiteral("abc"));
    QVERIFY(decoded.meta.bom);
    QCOMPARE(static_cast<int>(decoded.meta.encoding), static_cast<int>(TextEncoding::Utf8));
}

void TextFileFormatTest::crlfBytesPreserved()
{
    TextFileMeta meta;
    meta.encoding = TextEncoding::Utf8;
    meta.lineEnding = LineEnding::CrLf;
    const EncodedText encoded = encodeText(QStringLiteral("a\nb"), meta);
    QCOMPARE(encoded.bytes, QByteArray("a\r\nb"));
    const DecodedText decoded = decodeBytes(encoded.bytes);
    QCOMPARE(decoded.text, QStringLiteral("a\nb"));
    QCOMPARE(static_cast<int>(decoded.meta.lineEnding), static_cast<int>(LineEnding::CrLf));
}

void TextFileFormatTest::latin1Fallback()
{
    const QByteArray bytes("caf\xE9");
    const DecodedText decoded = decodeBytes(bytes);
    QCOMPARE(static_cast<int>(decoded.meta.encoding), static_cast<int>(TextEncoding::Latin1));
    QCOMPARE(decoded.text, QString::fromLatin1(bytes));
    const EncodedText encoded = encodeText(decoded.text, decoded.meta);
    QCOMPARE(encoded.bytes, bytes);
}

void TextFileFormatTest::latin1UpgradesWhenUnicode()
{
    TextFileMeta meta;
    meta.encoding = TextEncoding::Latin1;
    meta.lineEnding = LineEnding::Lf;
    const EncodedText encoded = encodeText(QStringLiteral("你好"), meta);
    QCOMPARE(static_cast<int>(encoded.meta.encoding), static_cast<int>(TextEncoding::Utf8));
    QVERIFY(!encoded.meta.bom);
    const DecodedText decoded = decodeBytes(encoded.bytes);
    QCOMPARE(decoded.text, QStringLiteral("你好"));
    QCOMPARE(static_cast<int>(decoded.meta.encoding), static_cast<int>(TextEncoding::Utf8));
}

void TextFileFormatTest::utf16LeRoundtrip()
{
    TextFileMeta meta;
    meta.encoding = TextEncoding::Utf16Le;
    meta.lineEnding = LineEnding::Lf;
    meta.bom = true;
    const QString text = QStringLiteral("hello\n世界");
    const EncodedText encoded = encodeText(text, meta);
    QVERIFY(encoded.bytes.startsWith("\xFF\xFE"));
    const DecodedText decoded = decodeBytes(encoded.bytes);
    QCOMPARE(decoded.text, text);
    QCOMPARE(static_cast<int>(decoded.meta.encoding), static_cast<int>(TextEncoding::Utf16Le));
    QVERIFY(decoded.meta.bom);
}

void TextFileFormatTest::invalidUtf8IsLatin1()
{
    const QByteArray bytes("\xFF");
    const DecodedText decoded = decodeBytes(bytes);
    QCOMPARE(static_cast<int>(decoded.meta.encoding), static_cast<int>(TextEncoding::Latin1));
}

void TextFileFormatTest::emptyFileMeta()
{
    const DecodedText decoded = decodeBytes({});
    QCOMPARE(decoded.text, QString());
    QCOMPARE(static_cast<int>(decoded.meta.encoding), static_cast<int>(TextEncoding::Utf8));
    QVERIFY(!decoded.meta.bom);
    QCOMPARE(static_cast<int>(decoded.meta.lineEnding), static_cast<int>(platformLineEnding()));
}

QTEST_GUILESS_MAIN(TextFileFormatTest)
#include "TextFileFormatTest.moc"
