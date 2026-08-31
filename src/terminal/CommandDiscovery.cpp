#include "terminal/CommandDiscovery.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <QtConcurrent/QtConcurrent>

CommandDiscovery::CommandDiscovery(QObject *parent)
    : QObject(parent)
{
}

void CommandDiscovery::initialize()
{
    seedBuiltins();
    loadCommandCache();
    scanSystemCommandsAsync();
}

void CommandDiscovery::seedBuiltins()
{
#ifdef Q_OS_WIN
    m_commands << "cd" << "dir" << "cls" << "copy" << "move" << "del" << "mkdir"
               << "rmdir" << "type" << "echo" << "set" << "path" << "exit"
               << "help" << "start" << "tasklist" << "taskkill" << "ipconfig"
               << "ping" << "netstat" << "systeminfo" << "chkdsk" << "diskpart"
               << "format" << "attrib" << "xcopy" << "robocopy" << "findstr"
               << "tree" << "fc" << "more" << "sort" << "find";

    m_commands << "git" << "npm" << "node" << "python" << "pip" << "cargo"
               << "rustc" << "cmake" << "make" << "gcc" << "g++" << "clang";

    m_arguments["cd"] = QStringList() << ".." << "." << "/d";
    m_arguments["dir"] = QStringList() << "/a" << "/b" << "/s" << "/p" << "/w";
    m_arguments["copy"] = QStringList() << "/y" << "/v" << "/z";
    m_arguments["del"] = QStringList() << "/p" << "/f" << "/s" << "/q";
    m_arguments["git"] = QStringList() << "clone" << "pull" << "push" << "commit"
                                       << "add" << "status" << "log" << "branch"
                                       << "checkout" << "merge" << "rebase" << "init";
    m_arguments["npm"] = QStringList() << "install" << "run" << "start" << "build"
                                       << "test" << "init" << "update" << "uninstall";
    m_arguments["pip"] = QStringList() << "install" << "uninstall" << "list" << "show"
                                       << "freeze" << "search" << "upgrade";
#else
    m_commands << "ls" << "cd" << "pwd" << "mkdir" << "rmdir" << "rm" << "cp"
               << "mv" << "touch" << "cat" << "grep" << "find" << "chmod"
               << "chown" << "ps" << "kill" << "top" << "df" << "du" << "tar"
               << "gzip" << "gunzip" << "wget" << "curl" << "ssh" << "scp"
               << "git" << "npm" << "node" << "python" << "pip" << "make"
               << "gcc" << "g++" << "sudo" << "apt" << "yum" << "systemctl";

    m_arguments["ls"] = QStringList() << "-l" << "-a" << "-h" << "-R" << "-t";
    m_arguments["rm"] = QStringList() << "-r" << "-f" << "-i" << "-v";
    m_arguments["cp"] = QStringList() << "-r" << "-i" << "-v" << "-p";
    m_arguments["chmod"] = QStringList() << "755" << "644" << "777" << "-R";
    m_arguments["git"] = QStringList() << "clone" << "pull" << "push" << "commit"
                                       << "add" << "status" << "log" << "branch"
                                       << "checkout" << "merge" << "rebase" << "init";
#endif
}

bool CommandDiscovery::hasArguments(const QString &command) const
{
    return m_arguments.contains(command);
}

QStringList CommandDiscovery::arguments(const QString &command) const
{
    return m_arguments.value(command);
}

void CommandDiscovery::ensureArguments(const QString &command)
{
    if (command.trimmed().isEmpty() || m_arguments.contains(command)) {
        return;
    }
    const QStringList cached = loadCachedArguments(command);
    if (!cached.isEmpty()) {
        m_arguments[command] = cached;
        return;
    }
    scanCommandArgumentsAsync(command);
}

QString CommandDiscovery::cacheDirectory()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!dir.isEmpty()) {
        QDir().mkpath(dir);
    }
    return dir;
}

QString CommandDiscovery::sanitizedCacheName(const QString &command)
{
    QString safeName = command;
    safeName.remove(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]")));
    return safeName;
}

void CommandDiscovery::loadCommandCache()
{
    const QString cacheDir = cacheDirectory();
    if (cacheDir.isEmpty()) {
        return;
    }
    const QString cacheFile = cacheDir + QDir::separator() + QStringLiteral("commands_cache.txt");

    QFile f(cacheFile);
    if (!f.exists() || !f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (!line.isEmpty() && !m_commands.contains(line)) {
            m_commands << line;
        }
    }
    m_commands.removeDuplicates();
    m_commands.sort(Qt::CaseInsensitive);
}

void CommandDiscovery::saveCommandCache()
{
    const QString cacheDir = cacheDirectory();
    if (cacheDir.isEmpty()) {
        return;
    }
    const QString cacheFile = cacheDir + QDir::separator() + QStringLiteral("commands_cache.txt");

    QFile f(cacheFile);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    QTextStream out(&f);
    for (const QString &cmd : m_commands) {
        out << cmd << "\n";
    }
}

QStringList CommandDiscovery::loadCachedArguments(const QString &command)
{
    QStringList result;
    const QString cacheDir = cacheDirectory();
    const QString safeName = sanitizedCacheName(command);
    if (cacheDir.isEmpty() || safeName.isEmpty()) {
        return result;
    }
    const QString cacheFile = cacheDir + QDir::separator() + QStringLiteral("args_%1.txt").arg(safeName);

    QFile f(cacheFile);
    if (!f.exists() || !f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return result;
    }
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            result << line;
        }
    }
    result.removeDuplicates();
    result.sort(Qt::CaseInsensitive);
    return result;
}

void CommandDiscovery::saveCachedArguments(const QString &command)
{
    if (!m_arguments.contains(command)) {
        return;
    }
    const QString cacheDir = cacheDirectory();
    const QString safeName = sanitizedCacheName(command);
    if (cacheDir.isEmpty() || safeName.isEmpty()) {
        return;
    }
    const QString cacheFile = cacheDir + QDir::separator() + QStringLiteral("args_%1.txt").arg(safeName);

    QFile f(cacheFile);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    QTextStream out(&f);
    const QStringList &args = m_arguments[command];
    for (const QString &a : args) {
        out << a << "\n";
    }
}

void CommandDiscovery::scanSystemCommandsAsync()
{
    auto future = QtConcurrent::run([]() -> QStringList {
        QStringList result;
        QString pathEnv = qEnvironmentVariable("PATH");
        QStringList paths = pathEnv.split(QDir::listSeparator(), Qt::SkipEmptyParts);
#ifndef Q_OS_WIN
        paths.append(QStringLiteral("/usr/local/bin"));
        paths.append(QStringLiteral("/opt/homebrew/bin"));
#endif
        QSet<QString> seen;
        for (const QString &p : paths) {
            QDir dir(p);
            if (!dir.exists()) {
                continue;
            }
            QDirIterator it(dir.absolutePath(), QDir::Files | QDir::NoSymLinks);
            while (it.hasNext()) {
                it.next();
                const QFileInfo fi = it.fileInfo();
#ifdef Q_OS_WIN
                const QString ext = fi.suffix().toLower();
                if (ext == QLatin1String("exe") || ext == QLatin1String("bat")
                    || ext == QLatin1String("cmd") || ext == QLatin1String("com")) {
                    seen.insert(fi.baseName());
                }
#else
                if (fi.isExecutable()) {
                    seen.insert(fi.fileName());
                }
#endif
            }
        }
        for (const QString &s : seen) {
            result << s;
        }
        result.sort(Qt::CaseInsensitive);
        return result;
    });

    auto *watcher = new QFutureWatcher<QStringList>(this);
    connect(watcher, &QFutureWatcher<QStringList>::finished, this, [this, watcher]() {
        const QStringList found = watcher->result();
        bool changed = false;
        for (const QString &c : found) {
            if (!m_commands.contains(c)) {
                m_commands << c;
                changed = true;
            }
        }
        if (changed) {
            m_commands.removeDuplicates();
            m_commands.sort(Qt::CaseInsensitive);
            saveCommandCache();
        }
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

void CommandDiscovery::scanCommandArgumentsAsync(const QString &command)
{
    if (command.trimmed().isEmpty()) {
        return;
    }

    auto future = QtConcurrent::run([command]() -> QStringList {
        QStringList result;
        auto tryRun = [&](const QString &prog, const QStringList &args) -> QString {
            QProcess p;
            p.start(prog, args);
            if (!p.waitForStarted(1000)) {
                return {};
            }
            p.waitForFinished(2000);
            const QByteArray out = p.readAllStandardOutput() + p.readAllStandardError();
            return QString::fromLocal8Bit(out).trimmed();
        };

        QString helpOut = tryRun(command, QStringList() << QStringLiteral("--help"));
        if (helpOut.isEmpty()) {
            helpOut = tryRun(command, QStringList() << QStringLiteral("-h"));
        }
        if (helpOut.isEmpty()) {
            helpOut = tryRun(command, QStringList() << QStringLiteral("help"));
        }
        if (helpOut.isEmpty()) {
#ifndef Q_OS_WIN
            helpOut = tryRun(QStringLiteral("man"), QStringList() << command);
#endif
        }

        if (helpOut.isEmpty()) {
            return result;
        }

        QStringList lines = helpOut.split('\n');
        if (lines.size() > 500) {
            lines = lines.mid(0, 500);
        }
        const QString snippet = lines.join('\n');

        QRegularExpression re(
            R"((?:^|[\s,;()\[\]])(-{1,2}[A-Za-z0-9][A-Za-z0-9._-]*(?:[= ][A-Za-z0-9_<>\\[\]-]+)?|/[A-Za-z0-9._-]+))");
        QSet<QString> seen;
        QRegularExpressionMatchIterator it = re.globalMatch(snippet);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            const QString token = m.captured(1).trimmed();
            if (!token.isEmpty()) {
                seen.insert(token);
            }
        }

        QStringList extras;
        for (const QString &t : seen) {
            if (t.startsWith('-') && t.length() > 2 && !t.startsWith(QLatin1String("--")) && !t.contains('=')) {
                for (int i = 1; i < t.length(); ++i) {
                    extras << QStringLiteral("-%1").arg(t.at(i));
                }
            }
        }
        for (const QString &e : extras) {
            seen.insert(e);
        }

        for (const QString &s : seen) {
            result << s;
        }
        result.removeDuplicates();
        result.sort(Qt::CaseInsensitive);
        return result;
    });

    auto *watcher = new QFutureWatcher<QStringList>(this);
    connect(watcher, &QFutureWatcher<QStringList>::finished, this, [this, watcher, command]() {
        const QStringList found = watcher->result();
        if (!found.isEmpty()) {
            m_arguments[command] = found;
            saveCachedArguments(command);
        }
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}
