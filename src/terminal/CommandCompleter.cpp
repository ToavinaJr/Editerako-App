#include "terminal/CommandCompleter.h"

#include "terminal/CommandDiscovery.h"
#include "terminal/CommandHistory.h"

#include <QDir>
#include <QFileInfo>

CommandCompleter::CommandCompleter(CommandDiscovery *discovery, const CommandHistory *history)
    : m_discovery(discovery)
    , m_history(history)
{
}

QStringList CommandCompleter::suggest(const QString &currentLine, const QString &workingDirectory)
{
    const QString currentCmd = currentLine.trimmed();
    if (currentCmd.isEmpty()) {
        return {};
    }

    const QStringList parts = currentCmd.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return {};
    }

    QStringList suggestions;
    if (parts.size() == 1) {
        suggestions = commandSuggestions(parts[0]);
    } else {
        const QString command = parts[0];
        const QString lastPart = parts.last();
        if (m_discovery) {
            m_discovery->ensureArguments(command);
        }
        suggestions = argumentSuggestions(command, lastPart);
        if (suggestions.isEmpty()) {
            suggestions = pathSuggestions(lastPart, workingDirectory);
        }
    }

    if (suggestions.size() > 15) {
        suggestions = suggestions.mid(0, 15);
    }
    return suggestions;
}

QStringList CommandCompleter::commandSuggestions(const QString &partial) const
{
    QStringList suggestions;
    const QString lowerPartial = partial.toLower();

    if (m_discovery) {
        for (const QString &cmd : m_discovery->commands()) {
            if (cmd.startsWith(lowerPartial, Qt::CaseInsensitive)) {
                suggestions << cmd;
            }
        }
    }

    if (m_history) {
        for (const QString &histCmd : m_history->entries()) {
            const QString firstWord = histCmd.split(' ').first();
            if (firstWord.startsWith(lowerPartial, Qt::CaseInsensitive)
                && !suggestions.contains(firstWord)) {
                suggestions << firstWord;
            }
        }
    }

    suggestions.sort(Qt::CaseInsensitive);
    return suggestions;
}

QStringList CommandCompleter::argumentSuggestions(const QString &command, const QString &partial) const
{
    QStringList suggestions;
    if (!m_discovery) {
        return suggestions;
    }
    const QStringList args = m_discovery->arguments(command);
    for (const QString &arg : args) {
        if (arg.startsWith(partial, Qt::CaseInsensitive)) {
            suggestions << arg;
        }
    }
    return suggestions;
}

QStringList CommandCompleter::pathSuggestions(const QString &partial, const QString &workingDirectory)
{
    QStringList suggestions;

    QString basePath = partial;
    QString searchPattern = QStringLiteral("*");

    int lastSlash = partial.lastIndexOf('/');
    if (lastSlash == -1) {
        lastSlash = partial.lastIndexOf('\\');
    }

    if (lastSlash != -1) {
        basePath = partial.left(lastSlash + 1);
        searchPattern = partial.mid(lastSlash + 1) + QLatin1Char('*');
    } else {
        basePath.clear();
        searchPattern = partial + QLatin1Char('*');
    }

    QDir searchDir;
    if (basePath.isEmpty()) {
        searchDir = QDir(workingDirectory);
    } else if (QDir::isAbsolutePath(basePath)) {
        searchDir = QDir(basePath);
    } else {
        searchDir = QDir(workingDirectory + QLatin1Char('/') + basePath);
    }

    if (!searchDir.exists()) {
        return suggestions;
    }

    const QStringList entries = searchDir.entryList(
        QStringList() << searchPattern,
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
        QDir::Name);

    for (const QString &entry : entries) {
        QString fullPath = basePath + entry;
        const QFileInfo info(searchDir.absoluteFilePath(entry));
        if (info.isDir()) {
            fullPath += QLatin1Char('/');
        }
        suggestions << fullPath;
    }

    return suggestions;
}
