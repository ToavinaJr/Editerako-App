#ifndef EDITERAKO_GITPROCESS_H
#define EDITERAKO_GITPROCESS_H

#include <QByteArray>
#include <QString>
#include <QStringList>

struct GitRunResult {
    int exitCode = -1;
    QByteArray standardOutput;
    QByteArray standardError;
    QString error;

    [[nodiscard]] bool ok() const { return error.isEmpty() && exitCode == 0; }
    [[nodiscard]] QString stderrText() const;
};

namespace GitProcess {
[[nodiscard]] QString gitExecutable();
[[nodiscard]] GitRunResult run(const QString &workingDirectory, const QStringList &args,
                               int timeoutMs = 30000);
}

#endif
