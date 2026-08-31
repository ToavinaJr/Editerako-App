#include "syntax/SyntaxHighlighter.h"

#include "core/Logging.h"
#include "syntax/HighlightQuery.h"
#include "syntax/TreeSitterDocument.h"

#include <QColor>
#include <QFont>
#include <QTextBlock>

namespace {

int utf8OffsetToUtf16(const QByteArray &utf8, const QString &text, uint32_t utf8Offset)
{
    if (utf8Offset == 0) {
        return 0;
    }
    if (utf8Offset >= static_cast<uint32_t>(utf8.size())) {
        return text.size();
    }
    return QString::fromUtf8(utf8.constData(), static_cast<int>(utf8Offset)).size();
}

} // namespace

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *document, LanguageId language)
    : QSyntaxHighlighter(static_cast<QObject *>(document))
    , m_language(language)
{
    if (!document) {
        qCWarning(lcSyntax) << "SyntaxHighlighter: document is nullptr";
        return;
    }

    setupFormats();
    m_treeDocument = new TreeSitterDocument(document, language, this);

    const TSLanguage *tsLang = LanguageRegistry::tsLanguage(language);
    const QByteArray source = LanguageRegistry::highlightQuerySource(language);
    if (tsLang && !source.isEmpty()) {
        m_query = std::make_unique<HighlightQuery>(tsLang, source);
        if (!m_query->isValid()) {
            qCWarning(lcSyntax) << "Highlight query failed for"
                                << LanguageRegistry::displayName(language)
                                << m_query->errorString();
            m_query.reset();
        }
    }

    setDocument(document);
}

SyntaxHighlighter::~SyntaxHighlighter() = default;

void SyntaxHighlighter::setupFormats()
{
    QTextCharFormat keyword;
    keyword.setForeground(QColor(86, 156, 214));
    keyword.setFontWeight(QFont::Bold);
    m_formats.insert(QStringLiteral("keyword"), keyword);

    QTextCharFormat type;
    type.setForeground(QColor(78, 201, 176));
    type.setFontWeight(QFont::Bold);
    m_formats.insert(QStringLiteral("type"), type);
    m_formats.insert(QStringLiteral("tag"), keyword);
    m_formats.insert(QStringLiteral("attribute"), type);

    QTextCharFormat string;
    string.setForeground(QColor(214, 157, 133));
    m_formats.insert(QStringLiteral("string"), string);

    QTextCharFormat comment;
    comment.setForeground(QColor(106, 153, 85));
    m_formats.insert(QStringLiteral("comment"), comment);

    QTextCharFormat number;
    number.setForeground(QColor(181, 206, 168));
    m_formats.insert(QStringLiteral("number"), number);
    m_formats.insert(QStringLiteral("constant"), number);

    QTextCharFormat preproc;
    preproc.setForeground(QColor(197, 134, 192));
    preproc.setFontItalic(true);
    m_formats.insert(QStringLiteral("preproc"), preproc);

    QTextCharFormat function;
    function.setForeground(QColor(220, 220, 170));
    function.setFontItalic(true);
    m_formats.insert(QStringLiteral("function"), function);

    QTextCharFormat variable;
    variable.setForeground(QColor(156, 220, 254));
    m_formats.insert(QStringLiteral("variable"), variable);
    m_formats.insert(QStringLiteral("property"), variable);

    QTextCharFormat parameter;
    parameter.setForeground(QColor(215, 186, 125));
    m_formats.insert(QStringLiteral("parameter"), parameter);

    QTextCharFormat punctuation;
    punctuation.setForeground(QColor(212, 212, 212));
    m_formats.insert(QStringLiteral("punctuation"), punctuation);

    QTextCharFormat op;
    op.setForeground(QColor(181, 206, 168));
    m_formats.insert(QStringLiteral("operator"), op);

    QTextCharFormat module;
    module.setForeground(QColor(255, 136, 0));
    module.setFontWeight(QFont::Bold);
    m_formats.insert(QStringLiteral("module"), module);
    m_formats.insert(QStringLiteral("namespace"), module);
}

QTextCharFormat SyntaxHighlighter::formatForCapture(const QString &captureName) const
{
    const QString family = captureName.section(QLatin1Char('.'), 0, 0);
    if (family == QLatin1String("keyword") && captureName.contains(QLatin1String("directive"))) {
        return m_formats.value(QStringLiteral("preproc"));
    }
    if (family == QLatin1String("variable") && captureName.contains(QLatin1String("parameter"))) {
        return m_formats.value(QStringLiteral("parameter"));
    }
    return m_formats.value(family);
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    if (!m_treeDocument || !m_treeDocument->isReady() || !m_query) {
        return;
    }

    uint32_t blockStart = 0;
    uint32_t blockEnd = 0;
    if (!m_treeDocument->utf8RangeForBlock(currentBlock().blockNumber(), &blockStart, &blockEnd)) {
        return;
    }
    if (blockStart >= blockEnd && text.isEmpty()) {
        return;
    }

    const QByteArray blockUtf8 = text.toUtf8();
    auto toLocalUtf16 = [&](uint32_t absByte) {
        if (absByte <= blockStart) {
            return 0;
        }
        return utf8OffsetToUtf16(blockUtf8, text, absByte - blockStart);
    };

    const TSNode root = m_treeDocument->rootNode();
    const QVector<HighlightQuery::Capture> caps =
        m_query->captures(root, blockStart, blockEnd, m_treeDocument->utf8());

    for (const HighlightQuery::Capture &cap : caps) {
        const QTextCharFormat fmt = formatForCapture(cap.name);
        if (!fmt.isValid()) {
            continue;
        }
        const int start = toLocalUtf16(cap.startByte);
        const int end = toLocalUtf16(cap.endByte);
        if (end > start) {
            setFormat(start, end - start, fmt);
        }
    }
}
