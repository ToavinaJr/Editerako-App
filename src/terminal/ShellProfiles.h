#ifndef EDITERAKO_SHELLPROFILES_H
#define EDITERAKO_SHELLPROFILES_H

#include <QString>
#include <QStringList>
#include <QVector>

struct TerminalProfile {
    QString id;
    QString name;
    QString shell;
};

[[nodiscard]] QString defaultShellPath();
[[nodiscard]] QVector<TerminalProfile> detectShellProfiles();
[[nodiscard]] QStringList shellCommandArguments(const QString &shell, const QString &command);

#endif
