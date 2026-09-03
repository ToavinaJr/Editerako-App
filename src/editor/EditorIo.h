#ifndef EDITERAKO_EDITORIO_H
#define EDITERAKO_EDITORIO_H

#include "core/TextFileFormat.h"

#include <QString>

struct TextLoadResult {
    bool ok = false;
    QString text;
    QString error;
    TextFileMeta meta;
};

struct TextSaveResult {
    bool ok = false;
    QString error;
    TextFileMeta meta;
};

[[nodiscard]] TextLoadResult readTextFile(const QString &path);
[[nodiscard]] TextSaveResult writeTextFile(const QString &path, const QString &lfText,
                                           const TextFileMeta &meta);
[[nodiscard]] bool diskMatches(const QString &path, const QString &lfText, const TextFileMeta &meta);

#endif
