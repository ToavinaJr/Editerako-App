#include "core/AtomicFile.h"

#include <QSaveFile>
#include <QTextStream>

bool writeTextAtomically(const QString &path, const QString &text, QString *errorOut)
{
    if (path.isEmpty()) {
        if (errorOut) {
            *errorOut = QStringLiteral("Empty path");
        }
        return false;
    }

    QSaveFile file(path);
    file.setDirectWriteFallback(true);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorOut) {
            *errorOut = file.errorString();
        }
        return false;
    }

    QTextStream out(&file);
    out << text;
    out.flush();
    if (file.error() != QFileDevice::NoError) {
        if (errorOut) {
            *errorOut = file.errorString();
        }
        return false;
    }

    if (!file.commit()) {
        if (errorOut) {
            *errorOut = file.errorString();
        }
        return false;
    }
    return true;
}
