#include "editor/features/BracketMatcher.h"

QChar closingBracket(QChar open)
{
    switch (open.unicode()) {
    case '(':
        return QLatin1Char(')');
    case '[':
        return QLatin1Char(']');
    case '{':
        return QLatin1Char('}');
    default:
        return {};
    }
}

namespace {

bool isOpener(QChar c)
{
    return c == QLatin1Char('(') || c == QLatin1Char('[') || c == QLatin1Char('{');
}

bool isCloser(QChar c)
{
    return c == QLatin1Char(')') || c == QLatin1Char(']') || c == QLatin1Char('}');
}

QChar openerFor(QChar close)
{
    switch (close.unicode()) {
    case ')':
        return QLatin1Char('(');
    case ']':
        return QLatin1Char('[');
    case '}':
        return QLatin1Char('{');
    default:
        return {};
    }
}

} // namespace

BracketMatch findBracketMatch(const QString &text, int position)
{
    BracketMatch match;
    if (text.isEmpty() || position < 0 || position > text.size()) {
        return match;
    }

    int index = -1;
    bool opening = false;
    if (position < text.size() && isOpener(text.at(position))) {
        index = position;
        opening = true;
    } else if (position > 0 && isOpener(text.at(position - 1))) {
        index = position - 1;
        opening = true;
    } else if (position < text.size() && isCloser(text.at(position))) {
        index = position;
        opening = false;
    } else if (position > 0 && isCloser(text.at(position - 1))) {
        index = position - 1;
        opening = false;
    }
    if (index < 0) {
        return match;
    }

    if (opening) {
        const QChar open = text.at(index);
        const QChar close = closingBracket(open);
        int depth = 0;
        for (int i = index; i < text.size(); ++i) {
            if (text.at(i) == open) {
                ++depth;
            } else if (text.at(i) == close) {
                --depth;
                if (depth == 0) {
                    match.open = index;
                    match.close = i;
                    return match;
                }
            }
        }
    } else {
        const QChar close = text.at(index);
        const QChar open = openerFor(close);
        int depth = 0;
        for (int i = index; i >= 0; --i) {
            if (text.at(i) == close) {
                ++depth;
            } else if (text.at(i) == open) {
                --depth;
                if (depth == 0) {
                    match.open = i;
                    match.close = index;
                    return match;
                }
            }
        }
    }
    return match;
}
