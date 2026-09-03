#ifndef EDITERAKO_EDITORIO_H
#define EDITERAKO_EDITORIO_H

#include <QString>

struct TextLoadResult {
    bool ok = false;
    QString text;
    QString error;
};

[[nodiscard]] TextLoadResult readTextFile(const QString &path);

#endif
