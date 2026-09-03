#ifndef EDITERAKO_TEXTFILEFORMAT_H
#define EDITERAKO_TEXTFILEFORMAT_H

#include <QByteArray>
#include <QString>
#include <QStringView>

enum class TextEncoding {
    Utf8,
    Utf16Le,
    Utf16Be,
    Latin1,
};

enum class LineEnding {
    Lf,
    CrLf,
    Cr,
};

struct TextFileMeta {
    TextEncoding encoding = TextEncoding::Utf8;
    LineEnding lineEnding = LineEnding::Lf;
    bool bom = false;

    [[nodiscard]] bool operator==(const TextFileMeta &other) const = default;
};

struct DecodedText {
    QString text;
    TextFileMeta meta;
};

struct EncodedText {
    QByteArray bytes;
    TextFileMeta meta;
};

[[nodiscard]] LineEnding platformLineEnding();
[[nodiscard]] TextFileMeta defaultTextFileMeta();

[[nodiscard]] QString encodingDisplayName(TextEncoding encoding, bool bom);
[[nodiscard]] QString lineEndingDisplayName(LineEnding ending);

[[nodiscard]] LineEnding detectLineEnding(QStringView text);
[[nodiscard]] QString normalizeToLf(QString text);
[[nodiscard]] QString applyLineEnding(const QString &lfText, LineEnding ending);
[[nodiscard]] bool isLatin1Representable(QStringView text);

[[nodiscard]] DecodedText decodeBytes(const QByteArray &bytes);
[[nodiscard]] EncodedText encodeText(const QString &lfText, TextFileMeta meta);

#endif
