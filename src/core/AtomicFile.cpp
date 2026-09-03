#include "core/AtomicFile.h"

#include <QSaveFile>

bool writeBytesAtomically(const QString &path, const QByteArray &bytes, QString *errorOut)
{
    if (path.isEmpty()) {
        if (errorOut) {
            *errorOut = QStringLiteral("Empty path");
        }
        return false;
    }

    QSaveFile file(path);
    file.setDirectWriteFallback(true);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorOut) {
            *errorOut = file.errorString();
        }
        return false;
    }

    if (file.write(bytes) != bytes.size()) {
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

bool writeTextAtomically(const QString &path, const QString &text, QString *errorOut)
{
    return writeBytesAtomically(path, text.toUtf8(), errorOut);
}
