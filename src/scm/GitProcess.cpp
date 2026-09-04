#include "scm/GitProcess.h"

#include "core/Logging.h"

#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

QString GitRunResult::stderrText() const
{
    const QString fromBytes = QString::fromUtf8(standardError).trimmed();
    if (!fromBytes.isEmpty()) {
        return fromBytes;
    }
    return error;
}

QString GitProcess::gitExecutable()
{
    const QString found = QStandardPaths::findExecutable(QStringLiteral("git"));
    if (!found.isEmpty()) {
        return found;
    }
    return QStandardPaths::findExecutable(QStringLiteral("git.exe"));
}

GitRunResult GitProcess::run(const QString &workingDirectory, const QStringList &args, int timeoutMs)
{
    GitRunResult result;
    const QString git = gitExecutable();
    if (git.isEmpty()) {
        result.error = QStringLiteral("git-not-found");
        return result;
    }
    if (workingDirectory.isEmpty() || !QFileInfo(workingDirectory).isDir()) {
        result.error = QStringLiteral("invalid-workdir");
        return result;
    }

    QProcess process;
    process.setWorkingDirectory(workingDirectory);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("GIT_OPTIONAL_LOCKS"), QStringLiteral("0"));
    process.setProcessEnvironment(env);

    QStringList fullArgs{
        QStringLiteral("-c"),
        QStringLiteral("core.quotepath=false"),
        QStringLiteral("-c"),
        QStringLiteral("color.ui=false"),
        QStringLiteral("--no-optional-locks"),
    };
    fullArgs += args;

    process.start(git, fullArgs);
    if (!process.waitForStarted(5000)) {
        result.error = process.errorString();
        qCWarning(lcScm) << "Git failed to start" << result.error;
        return result;
    }
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(2000);
        result.error = QStringLiteral("timeout");
        qCWarning(lcScm) << "Git timed out" << args;
        return result;
    }

    result.exitCode = process.exitCode();
    result.standardOutput = process.readAllStandardOutput();
    result.standardError = process.readAllStandardError();
    if (!result.ok()) {
        qCWarning(lcScm) << "Git exited" << result.exitCode << args << result.stderrText();
    }
    return result;
}
