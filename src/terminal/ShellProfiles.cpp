#include "terminal/ShellProfiles.h"

#include <QFileInfo>
#include <QStandardPaths>
#include <QtGlobal>

namespace {

QString findShell(const QString &name)
{
    const QString found = QStandardPaths::findExecutable(name);
    if (!found.isEmpty()) {
        return found;
    }
#ifdef Q_OS_WIN
    return QStandardPaths::findExecutable(name + QStringLiteral(".exe"));
#else
    return {};
#endif
}

void addIfFound(QVector<TerminalProfile> *out, const QString &id, const QString &name,
                const QString &executable)
{
    const QString path = findShell(executable);
    if (path.isEmpty()) {
        return;
    }
    out->append(TerminalProfile{id, name, path});
}

} // namespace

QString defaultShellPath()
{
#ifdef Q_OS_WIN
    const QString cmd = findShell(QStringLiteral("cmd"));
    return cmd.isEmpty() ? QStringLiteral("cmd.exe") : cmd;
#else
    const QString shell = qEnvironmentVariable("SHELL");
    if (!shell.isEmpty()) {
        return shell;
    }
    const QString bash = findShell(QStringLiteral("bash"));
    return bash.isEmpty() ? QStringLiteral("/bin/sh") : bash;
#endif
}

QVector<TerminalProfile> detectShellProfiles()
{
    QVector<TerminalProfile> out;
#ifdef Q_OS_WIN
    addIfFound(&out, QStringLiteral("cmd"), QStringLiteral("Command Prompt"),
               QStringLiteral("cmd"));
    addIfFound(&out, QStringLiteral("powershell"), QStringLiteral("Windows PowerShell"),
               QStringLiteral("powershell"));
    addIfFound(&out, QStringLiteral("pwsh"), QStringLiteral("PowerShell"),
               QStringLiteral("pwsh"));
#else
    addIfFound(&out, QStringLiteral("bash"), QStringLiteral("bash"), QStringLiteral("bash"));
    addIfFound(&out, QStringLiteral("zsh"), QStringLiteral("zsh"), QStringLiteral("zsh"));
    addIfFound(&out, QStringLiteral("sh"), QStringLiteral("sh"), QStringLiteral("sh"));
    const QString envShell = qEnvironmentVariable("SHELL");
    if (!envShell.isEmpty()) {
        bool already = false;
        for (const TerminalProfile &profile : out) {
            if (profile.shell == envShell) {
                already = true;
                break;
            }
        }
        if (!already) {
            out.prepend(TerminalProfile{QStringLiteral("default"), QFileInfo(envShell).fileName(),
                                        envShell});
        }
    }
#endif
    return out;
}

QStringList shellCommandArguments(const QString &shell, const QString &command)
{
#ifdef Q_OS_WIN
    const QString name = QFileInfo(shell).fileName().toLower();
    if (name == QLatin1String("powershell.exe") || name == QLatin1String("pwsh.exe")
        || name == QLatin1String("powershell") || name == QLatin1String("pwsh")) {
        return {QStringLiteral("-NoProfile"), QStringLiteral("-Command"), command};
    }
    return {QStringLiteral("/d"), QStringLiteral("/c"), command};
#else
    Q_UNUSED(shell)
    return {QStringLiteral("-c"), command};
#endif
}
