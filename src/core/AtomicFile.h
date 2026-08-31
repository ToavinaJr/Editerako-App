#ifndef EDITERAKO_ATOMICFILE_H
#define EDITERAKO_ATOMICFILE_H

#include <QString>

// Writes UTF-8 text via QSaveFile so a crash or I/O failure does not leave
// a truncated destination. Returns false and fills errorOut on failure.
bool writeTextAtomically(const QString &path, const QString &text, QString *errorOut = nullptr);

#endif
