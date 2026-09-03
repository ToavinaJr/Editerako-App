#ifndef EDITERAKO_ATOMICFILE_H
#define EDITERAKO_ATOMICFILE_H

#include <QByteArray>
#include <QString>

// Writes bytes via QSaveFile so a crash or I/O failure does not leave
// a truncated destination. Returns false and fills errorOut on failure.
bool writeBytesAtomically(const QString &path, const QByteArray &bytes, QString *errorOut = nullptr);

// UTF-8 without BOM; line endings are written as-is (no QIODevice::Text conversion).
bool writeTextAtomically(const QString &path, const QString &text, QString *errorOut = nullptr);

#endif
