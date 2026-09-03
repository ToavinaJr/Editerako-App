#include "editor/EditorIo.h"

#include <QFile>
#include <QTextStream>

TextLoadResult readTextFile(const QString &path)
{
    TextLoadResult result;
    if (path.isEmpty()) {
        result.error = QStringLiteral("Empty path");
        return result;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.error = file.errorString();
        return result;
    }

    QTextStream in(&file);
    result.text = in.readAll();
    result.ok = true;
    return result;
}
