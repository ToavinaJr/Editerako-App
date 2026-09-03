#include "editor/features/CommentOps.h"

#include "editor/features/IndentOps.h"

QStringList toggleLineComments(QStringList lines, const QString &marker)
{
    if (marker.isEmpty() || lines.isEmpty()) {
        return lines;
    }

    bool allCommented = true;
    for (const QString &line : lines) {
        if (line.trimmed().isEmpty()) {
            continue;
        }
        const QString rest = line.mid(leadingWhitespace(line).size());
        if (!rest.startsWith(marker)) {
            allCommented = false;
            break;
        }
    }

    const QString prefix = marker + QLatin1Char(' ');
    for (QString &line : lines) {
        if (line.trimmed().isEmpty()) {
            continue;
        }
        const QString ws = leadingWhitespace(line);
        const QString rest = line.mid(ws.size());
        if (allCommented) {
            if (rest.startsWith(prefix)) {
                line = ws + rest.mid(prefix.size());
            } else if (rest.startsWith(marker)) {
                line = ws + rest.mid(marker.size());
            }
        } else if (!rest.startsWith(marker)) {
            line = ws + prefix + rest;
        }
    }
    return lines;
}

QString toggleBlockComment(const QString &text, const QString &open, const QString &close)
{
    if (open.isEmpty() || close.isEmpty()) {
        return text;
    }
    const QString trimmed = text.trimmed();
    if (trimmed.startsWith(open) && trimmed.endsWith(close) && trimmed.size() >= open.size() + close.size()) {
        QString inner = text;
        const int start = inner.indexOf(open);
        const int end = inner.lastIndexOf(close);
        if (start >= 0 && end >= start + open.size()) {
            inner.remove(end, close.size());
            inner.remove(start, open.size());
            return inner;
        }
    }
    return open + text + close;
}
