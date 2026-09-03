#include "editor/features/AutoClosingPairs.h"

QChar closingPairFor(QChar typed)
{
    switch (typed.unicode()) {
    case '(':
        return QLatin1Char(')');
    case '[':
        return QLatin1Char(']');
    case '{':
        return QLatin1Char('}');
    case '"':
        return QLatin1Char('"');
    case '\'':
        return QLatin1Char('\'');
    default:
        return {};
    }
}

bool shouldSkipClosingPair(const QString &text, int position, QChar typed)
{
    if (position < 0 || position >= text.size()) {
        return false;
    }
    return text.at(position) == typed
        && (typed == QLatin1Char(')') || typed == QLatin1Char(']') || typed == QLatin1Char('}')
            || typed == QLatin1Char('"') || typed == QLatin1Char('\''));
}

bool shouldInsertClosingPair(const QString &text, int position, QChar typed)
{
    const QChar closer = closingPairFor(typed);
    if (closer.isNull()) {
        return false;
    }
    if (typed == QLatin1Char('\'') && position > 0) {
        const QChar prev = text.at(position - 1);
        if (prev.isLetterOrNumber() || prev == QLatin1Char('_')) {
            return false;
        }
    }
    if (position < text.size()) {
        const QChar next = text.at(position);
        if (next.isLetterOrNumber() || next == QLatin1Char('_')) {
            return false;
        }
    }
    return true;
}
