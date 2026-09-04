#include "lsp/LspHoverHtml.h"

#include <QRegularExpression>
#include <QSet>
#include <QStringList>

namespace {

const QSet<QString> &cppKeywords()
{
    static const QSet<QString> kWords = {
        QStringLiteral("alignas"),    QStringLiteral("alignof"),
        QStringLiteral("auto"),       QStringLiteral("bool"),
        QStringLiteral("break"),      QStringLiteral("case"),
        QStringLiteral("catch"),      QStringLiteral("char"),
        QStringLiteral("class"),      QStringLiteral("const"),
        QStringLiteral("consteval"),  QStringLiteral("constexpr"),
        QStringLiteral("continue"),   QStringLiteral("decltype"),
        QStringLiteral("default"),    QStringLiteral("delete"),
        QStringLiteral("do"),         QStringLiteral("double"),
        QStringLiteral("else"),       QStringLiteral("enum"),
        QStringLiteral("explicit"),   QStringLiteral("export"),
        QStringLiteral("extern"),     QStringLiteral("false"),
        QStringLiteral("float"),      QStringLiteral("for"),
        QStringLiteral("friend"),     QStringLiteral("goto"),
        QStringLiteral("if"),         QStringLiteral("inline"),
        QStringLiteral("int"),        QStringLiteral("long"),
        QStringLiteral("mutable"),    QStringLiteral("namespace"),
        QStringLiteral("new"),        QStringLiteral("noexcept"),
        QStringLiteral("nullptr"),    QStringLiteral("operator"),
        QStringLiteral("private"),    QStringLiteral("protected"),
        QStringLiteral("public"),     QStringLiteral("return"),
        QStringLiteral("short"),      QStringLiteral("signed"),
        QStringLiteral("sizeof"),     QStringLiteral("static"),
        QStringLiteral("struct"),     QStringLiteral("switch"),
        QStringLiteral("template"),   QStringLiteral("this"),
        QStringLiteral("throw"),      QStringLiteral("true"),
        QStringLiteral("try"),        QStringLiteral("typedef"),
        QStringLiteral("typename"),   QStringLiteral("union"),
        QStringLiteral("unsigned"),   QStringLiteral("using"),
        QStringLiteral("virtual"),    QStringLiteral("void"),
        QStringLiteral("volatile"),   QStringLiteral("while"),
        QStringLiteral("override"),   QStringLiteral("final"),
    };
    return kWords;
}

QString span(const QString &cls, const QString &escaped)
{
    return QStringLiteral("<span class=\"%1\">%2</span>").arg(cls, escaped);
}

bool nextNonSpaceIs(const QString &code, int index, QChar expected)
{
    while (index < code.size() && code.at(index).isSpace()) {
        ++index;
    }
    return index < code.size() && code.at(index) == expected;
}

} // namespace

QString lspEscapeHtml(const QString &text)
{
    QString out;
    out.reserve(text.size() + 8);
    for (const QChar ch : text) {
        if (ch == QLatin1Char('&')) {
            out += QStringLiteral("&amp;");
        } else if (ch == QLatin1Char('<')) {
            out += QStringLiteral("&lt;");
        } else if (ch == QLatin1Char('>')) {
            out += QStringLiteral("&gt;");
        } else {
            out += ch;
        }
    }
    return out;
}

QString lspHighlightCppHtml(const QString &code)
{
    QString html;
    html.reserve(code.size() * 2);
    const int n = code.size();
    int i = 0;
    while (i < n) {
        const QChar ch = code.at(i);
        if (ch.isSpace()) {
            html += lspEscapeHtml(QString(ch));
            ++i;
            continue;
        }
        if (ch == QLatin1Char('/') && i + 1 < n && code.at(i + 1) == QLatin1Char('/')) {
            const int start = i;
            i = code.indexOf(QLatin1Char('\n'), i);
            if (i < 0) {
                i = n;
            }
            html += span(QStringLiteral("hc"), lspEscapeHtml(code.mid(start, i - start)));
            continue;
        }
        if (ch == QLatin1Char('"') || ch == QLatin1Char('\'')) {
            const QChar quote = ch;
            const int start = i;
            ++i;
            while (i < n && code.at(i) != quote) {
                if (code.at(i) == QLatin1Char('\\') && i + 1 < n) {
                    i += 2;
                    continue;
                }
                ++i;
            }
            if (i < n) {
                ++i;
            }
            html += span(QStringLiteral("hs"), lspEscapeHtml(code.mid(start, i - start)));
            continue;
        }
        if (ch.isDigit()) {
            const int start = i;
            while (i < n && (code.at(i).isLetterOrNumber() || code.at(i) == QLatin1Char('.'))) {
                ++i;
            }
            html += span(QStringLiteral("hn"), lspEscapeHtml(code.mid(start, i - start)));
            continue;
        }
        if (ch.isLetter() || ch == QLatin1Char('_')) {
            const int start = i;
            while (i < n && (code.at(i).isLetterOrNumber() || code.at(i) == QLatin1Char('_'))) {
                ++i;
            }
            const QString tok = code.mid(start, i - start);
            QString cls = QStringLiteral("hi");
            if (cppKeywords().contains(tok)) {
                cls = QStringLiteral("hk");
            } else if (tok.at(0).isUpper()) {
                cls = QStringLiteral("ht");
            } else if (nextNonSpaceIs(code, i, QLatin1Char('('))) {
                cls = QStringLiteral("hf");
            }
            html += span(cls, lspEscapeHtml(tok));
            continue;
        }
        html += span(QStringLiteral("hp"), lspEscapeHtml(QString(ch)));
        ++i;
    }
    return html;
}

QString lspMarkupToHtml(const QString &markdown, bool withDefinitionLink)
{
    QString rest = markdown;
    rest.replace(QLatin1Char('\r'), QString());
    QStringList stashed;

    const auto stash = [&stashed](const QString &html) {
        const QString token = QStringLiteral("\x1ePH%1\x1e").arg(stashed.size());
        stashed.append(html);
        return token;
    };

    const auto apply = [](const QString &input, const QRegularExpression &re,
                          const auto &fn) {
        QString out;
        int last = 0;
        QRegularExpressionMatchIterator it = re.globalMatch(input);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            out += input.mid(last, match.capturedStart() - last);
            out += fn(match);
            last = match.capturedEnd();
        }
        out += input.mid(last);
        return out;
    };

    rest = apply(rest, QRegularExpression(QStringLiteral("```(\\w*)\\n([\\s\\S]*?)```")),
                 [&](const QRegularExpressionMatch &match) {
                     const QString lang = match.captured(1).toLower();
                     const QString body = match.captured(2).trimmed();
                     const QString inner =
                         (lang == QLatin1String("cpp") || lang == QLatin1String("c") || lang.isEmpty())
                         ? lspHighlightCppHtml(body)
                         : lspEscapeHtml(body);
                     return stash(QStringLiteral("<pre class=\"hover-code\">%1</pre>").arg(inner));
                 });

    rest = apply(rest, QRegularExpression(QStringLiteral("`([^`]+)`")),
                 [&](const QRegularExpressionMatch &match) {
                     return stash(QStringLiteral("<code class=\"hover-code\">%1</code>")
                                      .arg(lspHighlightCppHtml(match.captured(1))));
                 });

    rest = lspEscapeHtml(rest);

    QString withBold;
    bool bold = false;
    const QStringList boldParts = rest.split(QStringLiteral("**"));
    for (int i = 0; i < boldParts.size(); ++i) {
        if (i > 0) {
            withBold += bold ? QStringLiteral("</b>") : QStringLiteral("<b>");
            bold = !bold;
        }
        withBold += boldParts.at(i);
    }
    if (bold) {
        withBold += QStringLiteral("</b>");
    }
    rest = withBold;

    const QStringList lines = rest.split(QLatin1Char('\n'));
    QString html;
    bool inList = false;
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed == QLatin1String("---") || trimmed == QLatin1String("***")) {
            if (inList) {
                html += QStringLiteral("</ul>");
                inList = false;
            }
            html += QStringLiteral("<hr/>");
            continue;
        }
        if (trimmed.startsWith(QLatin1String("### "))) {
            if (inList) {
                html += QStringLiteral("</ul>");
                inList = false;
            }
            html += QStringLiteral("<div class=\"hover-title\">%1</div>").arg(trimmed.mid(4));
            continue;
        }
        if (trimmed.startsWith(QLatin1String("- ")) || trimmed.startsWith(QLatin1String("* "))) {
            if (!inList) {
                html += QStringLiteral("<ul>");
                inList = true;
            }
            html += QStringLiteral("<li>%1</li>").arg(trimmed.mid(2));
            continue;
        }
        if (inList) {
            html += QStringLiteral("</ul>");
            inList = false;
        }
        if (trimmed.isEmpty()) {
            html += QStringLiteral("<br/>");
            continue;
        }
        html += QStringLiteral("<div>%1</div>").arg(line);
    }
    if (inList) {
        html += QStringLiteral("</ul>");
    }

    for (int i = 0; i < stashed.size(); ++i) {
        html.replace(QStringLiteral("\x1ePH%1\x1e").arg(i), stashed.at(i));
    }

    if (withDefinitionLink) {
        html += QStringLiteral(
            "<p><a class=\"hover-def\" href=\"editerako:definition\">Go to Definition</a></p>");
    }
    return html;
}
