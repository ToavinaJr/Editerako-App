#include "core/TextFileFormat.h"

#include <QStringConverter>
#include <QStringDecoder>
#include <QStringEncoder>
#include <QtGlobal>

namespace {

QByteArray utf8Bom()
{
    return QByteArray("\xEF\xBB\xBF", 3);
}

QByteArray utf16LeBom()
{
    return QByteArray("\xFF\xFE", 2);
}

QByteArray utf16BeBom()
{
    return QByteArray("\xFE\xFF", 2);
}

QByteArray utf32LeBom()
{
    return QByteArray("\xFF\xFE\x00\x00", 4);
}

QByteArray utf32BeBom()
{
    return QByteArray("\x00\x00\xFE\xFF", 4);
}

bool isValidUtf8(const QByteArray &bytes)
{
    QStringDecoder decoder(QStringDecoder::Utf8);
    const QString text = decoder(bytes);
    QStringEncoder encoder(QStringEncoder::Utf8);
    return encoder(text) == bytes;
}

QString decodeWith(QStringConverter::Encoding encoding, const QByteArray &bytes)
{
    QStringDecoder decoder(encoding);
    return decoder(bytes);
}

} // namespace

LineEnding platformLineEnding()
{
#ifdef Q_OS_WIN
    return LineEnding::CrLf;
#else
    return LineEnding::Lf;
#endif
}

TextFileMeta defaultTextFileMeta()
{
    TextFileMeta meta;
    meta.encoding = TextEncoding::Utf8;
    meta.lineEnding = platformLineEnding();
    meta.bom = false;
    return meta;
}

QString encodingDisplayName(TextEncoding encoding, bool bom)
{
    switch (encoding) {
    case TextEncoding::Utf8:
        return bom ? QStringLiteral("UTF-8 BOM") : QStringLiteral("UTF-8");
    case TextEncoding::Utf16Le:
        return QStringLiteral("UTF-16 LE");
    case TextEncoding::Utf16Be:
        return QStringLiteral("UTF-16 BE");
    case TextEncoding::Latin1:
        return QStringLiteral("ISO-8859-1");
    }
    return QStringLiteral("UTF-8");
}

QString lineEndingDisplayName(LineEnding ending)
{
    switch (ending) {
    case LineEnding::Lf:
        return QStringLiteral("LF");
    case LineEnding::CrLf:
        return QStringLiteral("CRLF");
    case LineEnding::Cr:
        return QStringLiteral("CR");
    }
    return QStringLiteral("LF");
}

LineEnding detectLineEnding(QStringView text)
{
    int lf = 0;
    int crlf = 0;
    int cr = 0;
    for (qsizetype i = 0; i < text.size(); ++i) {
        if (text[i] == QLatin1Char('\r')) {
            if (i + 1 < text.size() && text[i + 1] == QLatin1Char('\n')) {
                ++crlf;
                ++i;
            } else {
                ++cr;
            }
        } else if (text[i] == QLatin1Char('\n')) {
            ++lf;
        }
    }

    if (crlf == 0 && lf == 0 && cr == 0) {
        return platformLineEnding();
    }
    if (crlf >= lf && crlf >= cr) {
        return LineEnding::CrLf;
    }
    if (cr > lf) {
        return LineEnding::Cr;
    }
    return LineEnding::Lf;
}

QString normalizeToLf(QString text)
{
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    return text;
}

QString applyLineEnding(const QString &lfText, LineEnding ending)
{
    switch (ending) {
    case LineEnding::CrLf: {
        QString out = lfText;
        out.replace(QLatin1Char('\n'), QStringLiteral("\r\n"));
        return out;
    }
    case LineEnding::Cr: {
        QString out = lfText;
        out.replace(QLatin1Char('\n'), QLatin1Char('\r'));
        return out;
    }
    case LineEnding::Lf:
        break;
    }
    return lfText;
}

bool isLatin1Representable(QStringView text)
{
    for (const QChar ch : text) {
        if (ch.unicode() > 0xFF) {
            return false;
        }
    }
    return true;
}

DecodedText decodeBytes(const QByteArray &bytes)
{
    DecodedText result;
    result.meta = defaultTextFileMeta();
    QString raw;

    if (bytes.startsWith(utf32LeBom())) {
        raw = decodeWith(QStringConverter::Utf32LE, bytes);
        result.meta.encoding = TextEncoding::Utf8;
        result.meta.bom = false;
    } else if (bytes.startsWith(utf32BeBom())) {
        raw = decodeWith(QStringConverter::Utf32BE, bytes);
        result.meta.encoding = TextEncoding::Utf8;
        result.meta.bom = false;
    } else if (bytes.startsWith(utf8Bom())) {
        raw = decodeWith(QStringConverter::Utf8, bytes.mid(3));
        result.meta.encoding = TextEncoding::Utf8;
        result.meta.bom = true;
    } else if (bytes.startsWith(utf16LeBom())) {
        raw = decodeWith(QStringConverter::Utf16LE, bytes.mid(2));
        result.meta.encoding = TextEncoding::Utf16Le;
        result.meta.bom = true;
    } else if (bytes.startsWith(utf16BeBom())) {
        raw = decodeWith(QStringConverter::Utf16BE, bytes.mid(2));
        result.meta.encoding = TextEncoding::Utf16Be;
        result.meta.bom = true;
    } else if (isValidUtf8(bytes)) {
        raw = decodeWith(QStringConverter::Utf8, bytes);
        result.meta.encoding = TextEncoding::Utf8;
        result.meta.bom = false;
    } else {
        raw = decodeWith(QStringConverter::Latin1, bytes);
        result.meta.encoding = TextEncoding::Latin1;
        result.meta.bom = false;
    }

    result.meta.lineEnding = detectLineEnding(raw);
    result.text = normalizeToLf(raw);
    return result;
}

EncodedText encodeText(const QString &lfText, TextFileMeta meta)
{
    EncodedText result;
    if (meta.encoding == TextEncoding::Latin1 && !isLatin1Representable(lfText)) {
        meta.encoding = TextEncoding::Utf8;
        meta.bom = false;
    }
    result.meta = meta;

    const QString withEol = applyLineEnding(lfText, meta.lineEnding);
    QByteArray payload;
    switch (meta.encoding) {
    case TextEncoding::Utf16Le: {
        QStringEncoder encoder(QStringEncoder::Utf16LE);
        payload = encoder(withEol);
        result.bytes = utf16LeBom() + payload;
        result.meta.bom = true;
        return result;
    }
    case TextEncoding::Utf16Be: {
        QStringEncoder encoder(QStringEncoder::Utf16BE);
        payload = encoder(withEol);
        result.bytes = utf16BeBom() + payload;
        result.meta.bom = true;
        return result;
    }
    case TextEncoding::Latin1: {
        QStringEncoder encoder(QStringEncoder::Latin1);
        payload = encoder(withEol);
        result.bytes = payload;
        return result;
    }
    case TextEncoding::Utf8: {
        QStringEncoder encoder(QStringEncoder::Utf8);
        payload = encoder(withEol);
        if (meta.bom) {
            result.bytes = utf8Bom() + payload;
        } else {
            result.bytes = payload;
        }
        return result;
    }
    }
    result.bytes = withEol.toUtf8();
    return result;
}
