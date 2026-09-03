#include "project/GitIgnore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

namespace {

QString globToRegex(QString pattern)
{
    pattern.replace(QLatin1Char('\\'), QLatin1Char('/'));
    QString out;
    out.reserve(pattern.size() * 2);
    for (int i = 0; i < pattern.size(); ++i) {
        const QChar c = pattern.at(i);
        if (c == QLatin1Char('*')) {
            if (i + 1 < pattern.size() && pattern.at(i + 1) == QLatin1Char('*')) {
                out += QStringLiteral(".*");
                ++i;
                if (i + 1 < pattern.size() && pattern.at(i + 1) == QLatin1Char('/')) {
                    ++i;
                }
            } else {
                out += QStringLiteral("[^/]*");
            }
        } else if (c == QLatin1Char('?')) {
            out += QStringLiteral("[^/]");
        } else if (QStringLiteral(".^$+()[]{}|\\").contains(c)) {
            out += QLatin1Char('\\');
            out += c;
        } else {
            out += c;
        }
    }
    return out;
}

} // namespace

bool globMatches(const QString &pattern, const QString &relativePath)
{
    const QString trimmed = pattern.trimmed();
    if (trimmed.isEmpty()) {
        return true;
    }
    const QString path = QDir::fromNativeSeparators(relativePath);
    const QString fileName = QFileInfo(path).fileName();
    const QStringList parts = trimmed.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (QString part : parts) {
        part = part.trimmed();
        if (part.isEmpty()) {
            continue;
        }
        if (QDir::match(part, path) || QDir::match(part, fileName)) {
            return true;
        }
        if (!part.contains(QLatin1Char('/')) && QDir::match(QStringLiteral("*/") + part, path)) {
            return true;
        }
    }
    return false;
}

GitIgnore GitIgnore::fromText(const QString &text)
{
    GitIgnore ignore;
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("\r\n|\n|\r")));
    for (QString line : lines) {
        if (line.endsWith(QLatin1Char('\r'))) {
            line.chop(1);
        }
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        Rule rule;
        if (line.startsWith(QLatin1Char('!'))) {
            rule.negation = true;
            line = line.mid(1);
        }
        if (line.endsWith(QLatin1Char('/'))) {
            rule.directoryOnly = true;
            line.chop(1);
        }
        line.replace(QLatin1Char('\\'), QLatin1Char('/'));
        const bool rooted = line.startsWith(QLatin1Char('/'));
        if (rooted) {
            line = line.mid(1);
        }
        const bool anywhere = !rooted && !line.contains(QLatin1Char('/'));
        QString body = globToRegex(line);
        QString pattern;
        if (anywhere) {
            pattern = QStringLiteral("(?:^|.*/)%1(?:/.*)?$").arg(body);
        } else {
            pattern = QStringLiteral("^%1(?:/.*)?$").arg(body);
        }
        rule.regex = QRegularExpression(pattern, QRegularExpression::DotMatchesEverythingOption);
        if (rule.regex.isValid()) {
            ignore.m_rules.append(rule);
        }
    }
    return ignore;
}

GitIgnore GitIgnore::loadFromWorkspace(const QString &workspaceRoot)
{
    if (workspaceRoot.isEmpty()) {
        return {};
    }
    QFile file(QDir(workspaceRoot).filePath(QStringLiteral(".gitignore")));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    QTextStream stream(&file);
    return fromText(stream.readAll());
}

bool GitIgnore::isIgnored(const QString &relativePath, bool isDirectory) const
{
    const QString path = QDir::fromNativeSeparators(relativePath);
    if (path.isEmpty()) {
        return false;
    }
    const QStringList parts = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (parts.contains(QStringLiteral(".git"))) {
        return true;
    }

    bool ignored = false;
    for (const Rule &rule : m_rules) {
        if (rule.directoryOnly && !isDirectory) {
            const int slash = path.lastIndexOf(QLatin1Char('/'));
            const QString parent = slash < 0 ? QString() : path.left(slash);
            if (parent.isEmpty() || !rule.regex.match(parent).hasMatch()) {
                continue;
            }
            ignored = !rule.negation;
            continue;
        }
        if (rule.regex.match(path).hasMatch()) {
            ignored = !rule.negation;
        }
    }
    return ignored;
}
