#include "editor/EditorIo.h"

#include "core/AtomicFile.h"

#include <QFile>

TextLoadResult readTextFile(const QString &path)
{
    TextLoadResult result;
    if (path.isEmpty()) {
        result.error = QStringLiteral("Empty path");
        return result;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = file.errorString();
        return result;
    }

    const DecodedText decoded = decodeBytes(file.readAll());
    result.text = decoded.text;
    result.meta = decoded.meta;
    result.ok = true;
    return result;
}

TextSaveResult writeTextFile(const QString &path, const QString &lfText, const TextFileMeta &meta)
{
    TextSaveResult result;
    const EncodedText encoded = encodeText(lfText, meta);
    result.meta = encoded.meta;
    if (!writeBytesAtomically(path, encoded.bytes, &result.error)) {
        return result;
    }
    result.ok = true;
    return result;
}

bool diskMatches(const QString &path, const QString &lfText, const TextFileMeta &meta)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    return file.readAll() == encodeText(lfText, meta).bytes;
}
