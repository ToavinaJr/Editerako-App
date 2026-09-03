#include "editor/features/IndentOps.h"

#include <QtGlobal>
#include <algorithm>

QString indentUnit(int tabSize, bool insertSpaces)
{
    if (insertSpaces) {
        return QString(qMax(1, tabSize), QLatin1Char(' '));
    }
    return QString(QLatin1Char('\t'));
}

QString leadingWhitespace(const QString &line)
{
    int i = 0;
    while (i < line.size() && (line.at(i) == QLatin1Char(' ') || line.at(i) == QLatin1Char('\t'))) {
        ++i;
    }
    return line.left(i);
}

void indentLines(QStringList *lines, int tabSize, bool insertSpaces)
{
    if (!lines) {
        return;
    }
    const QString unit = indentUnit(tabSize, insertSpaces);
    for (QString &line : *lines) {
        if (line.isEmpty()) {
            continue;
        }
        line.prepend(unit);
    }
}

void outdentLines(QStringList *lines, int tabSize, bool insertSpaces)
{
    Q_UNUSED(insertSpaces)
    if (!lines) {
        return;
    }
    const int width = qMax(1, tabSize);
    for (QString &line : *lines) {
        if (line.startsWith(QLatin1Char('\t'))) {
            line.remove(0, 1);
            continue;
        }
        int n = 0;
        while (n < line.size() && n < width && line.at(n) == QLatin1Char(' ')) {
            ++n;
        }
        if (n > 0) {
            line.remove(0, n);
        }
    }
}

QString smartIndentPrefix(const QString &currentLine, int tabSize, bool insertSpaces)
{
    QString prefix = leadingWhitespace(currentLine);
    const QString trimmed = currentLine.trimmed();
    if (trimmed.endsWith(QLatin1Char('{')) || trimmed.endsWith(QLatin1Char('('))
        || trimmed.endsWith(QLatin1Char('[')) || trimmed.endsWith(QLatin1Char(':'))) {
        prefix += indentUnit(tabSize, insertSpaces);
    }
    return prefix;
}

QString convertIndentation(const QString &text, bool toSpaces, int tabSize)
{
    const int width = qMax(1, tabSize);
    const QStringList lines = text.split(QLatin1Char('\n'));
    QStringList out;
    out.reserve(lines.size());
    for (const QString &line : lines) {
        const QString ws = leadingWhitespace(line);
        int columns = 0;
        for (QChar c : ws) {
            if (c == QLatin1Char('\t')) {
                columns += width;
            } else {
                ++columns;
            }
        }
        QString indent;
        if (toSpaces) {
            indent = QString(columns, QLatin1Char(' '));
        } else {
            indent = QString(columns / width, QLatin1Char('\t'));
            indent += QString(columns % width, QLatin1Char(' '));
        }
        out.append(indent + line.mid(ws.size()));
    }
    return out.join(QLatin1Char('\n'));
}

QString trimTrailingWhitespace(const QString &text)
{
    const QStringList lines = text.split(QLatin1Char('\n'));
    QStringList out;
    out.reserve(lines.size());
    for (QString line : lines) {
        while (!line.isEmpty()
               && (line.back() == QLatin1Char(' ') || line.back() == QLatin1Char('\t'))) {
            line.chop(1);
        }
        out.append(line);
    }
    return out.join(QLatin1Char('\n'));
}

QString sortLinesText(const QString &text)
{
    QStringList lines = text.split(QLatin1Char('\n'));
    std::sort(lines.begin(), lines.end());
    return lines.join(QLatin1Char('\n'));
}
