#ifndef EDITERAKO_FILEKIND_H
#define EDITERAKO_FILEKIND_H

#include <QString>

enum class FileKind {
    Text,
    Pdf,
    Image,
    Svg,
    Csv,
    Unsupported,
};

[[nodiscard]] FileKind fileKindForPath(const QString &path);
[[nodiscard]] bool isMarkdownPath(const QString &path);

#endif
