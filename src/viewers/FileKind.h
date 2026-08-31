#ifndef EDITERAKO_FILEKIND_H
#define EDITERAKO_FILEKIND_H

#include <QString>

enum class FileKind {
    Text,
    Pdf,
    Image,
    Unsupported,
};

[[nodiscard]] FileKind fileKindForPath(const QString &path);

#endif
