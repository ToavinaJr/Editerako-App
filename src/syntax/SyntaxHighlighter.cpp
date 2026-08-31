#include "syntax/SyntaxHighlighter.h"

#include "core/Logging.h"
#include "syntax/TreeSitterDocument.h"

#include <QColor>
#include <QFont>
#include <QRegularExpression>
#include <QStringList>
#include <QTextBlock>
#include <cstring>

namespace {

const QStringList &cppKeywords()
{
    static const QStringList keywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break",
        "case", "catch", "char", "char16_t", "char32_t", "class", "compl", "const", "constexpr",
        "const_cast", "continue", "decltype", "default", "delete", "do", "double", "dynamic_cast",
        "else", "enum", "explicit", "export", "extern", "false", "float", "for", "friend", "goto",
        "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq",
        "nullptr", "operator", "or", "or_eq", "private", "protected", "public", "register",
        "reinterpret_cast", "return", "short", "signed", "sizeof", "static", "static_assert",
        "static_cast", "struct", "switch", "template", "this", "thread_local", "throw", "true",
        "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void",
        "volatile", "wchar_t", "while", "xor", "xor_eq"};
    return keywords;
}

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
    setDocument(document);
}

void SyntaxHighlighter::setupFormats()
{
    m_keywordFormat.setForeground(QColor(86, 156, 214));
    m_keywordFormat.setFontWeight(QFont::Bold);

    m_typeFormat.setForeground(QColor(78, 201, 176));
    m_typeFormat.setFontWeight(QFont::Bold);

    m_stringFormat.setForeground(QColor(214, 157, 133));
    m_commentFormat.setForeground(QColor(106, 153, 85));
    m_numberFormat.setForeground(QColor(181, 206, 168));

    m_preprocFormat.setForeground(QColor(197, 134, 192));
    m_preprocFormat.setFontItalic(true);

    m_functionFormat.setForeground(QColor(220, 220, 170));
    m_functionFormat.setFontItalic(true);

    m_variableFormat.setForeground(QColor(156, 220, 254));
    m_parameterFormat.setForeground(QColor(215, 186, 125));
    m_punctuationFormat.setForeground(QColor(212, 212, 212));
    m_operatorFormat.setForeground(QColor(181, 206, 168));

    m_namespaceFormat.setForeground(QColor(255, 136, 0));
    m_namespaceFormat.setFontWeight(QFont::Bold);
}

void SyntaxHighlighter::applyNodeFormat(const char *type, int start, int length)
{
    if (!type || length <= 0) {
        return;
    }

    if (m_language == LanguageId::Html) {
        if (std::strcmp(type, "tag_name") == 0) {
            setFormat(start, length, m_keywordFormat);
        } else if (std::strcmp(type, "attribute_name") == 0) {
            setFormat(start, length, m_typeFormat);
        } else if (std::strcmp(type, "quoted_attribute_value") == 0
                   || std::strcmp(type, "attribute_value") == 0
                   || std::strcmp(type, "string") == 0) {
            setFormat(start, length, m_stringFormat);
        } else if (std::strcmp(type, "comment") == 0) {
            setFormat(start, length, m_commentFormat);
        }
        return;
    }

    if (std::strncmp(type, "preproc", 7) == 0) {
        setFormat(start, length, m_preprocFormat);
    } else if (std::strcmp(type, "comment") == 0) {
        setFormat(start, length, m_commentFormat);
    } else if (std::strcmp(type, "string_literal") == 0
               || std::strcmp(type, "raw_string_literal") == 0
               || std::strcmp(type, "char_literal") == 0) {
        setFormat(start, length, m_stringFormat);
    } else if (std::strcmp(type, "number_literal") == 0) {
        setFormat(start, length, m_numberFormat);
    } else if (std::strcmp(type, "primitive_type") == 0
               || std::strcmp(type, "type_identifier") == 0) {
        setFormat(start, length, m_typeFormat);
    } else if (std::strcmp(type, "function_definition") == 0
               || std::strcmp(type, "function_declarator") == 0
               || std::strcmp(type, "operator_cast") == 0
               || std::strcmp(type, "operator_cast_definition") == 0
               || std::strcmp(type, "function") == 0
               || std::strcmp(type, "call_expression") == 0) {
        setFormat(start, length, m_functionFormat);
    } else if (std::strcmp(type, "identifier") == 0) {
        setFormat(start, length, m_variableFormat);
    } else if (std::strcmp(type, "parameter_declaration") == 0) {
        setFormat(start, length, m_parameterFormat);
    } else if (std::strcmp(type, "namespace") == 0
               || std::strcmp(type, "namespace_definition") == 0) {
        setFormat(start, length, m_namespaceFormat);
    } else if (std::strcmp(type, "class_specifier") == 0
               || std::strcmp(type, "struct_specifier") == 0) {
        setFormat(start, length, m_keywordFormat);
    } else if (std::strcmp(type, "operator_name") == 0) {
        setFormat(start, length, m_operatorFormat);
    } else if (std::strcmp(type, "{") == 0 || std::strcmp(type, "}") == 0
               || std::strcmp(type, "(") == 0 || std::strcmp(type, ")") == 0
               || std::strcmp(type, "[") == 0 || std::strcmp(type, "]") == 0
               || std::strcmp(type, ";") == 0 || std::strcmp(type, ",") == 0) {
        setFormat(start, length, m_punctuationFormat);
    }
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    if (!m_treeDocument || !m_treeDocument->isReady()) {
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

    m_treeDocument->visitOverlapping(blockStart, blockEnd, [&](TSNode node) {
        const uint32_t nodeStart = ts_node_start_byte(node);
        const uint32_t nodeEnd = ts_node_end_byte(node);
        const uint32_t clippedStart = qMax(nodeStart, blockStart);
        const uint32_t clippedEnd = qMin(nodeEnd, blockEnd);
        if (clippedEnd <= clippedStart) {
            return;
        }

        const int start = toLocalUtf16(clippedStart);
        const int end = toLocalUtf16(clippedEnd);
        applyNodeFormat(ts_node_type(node), start, end - start);
    });

    if (m_language != LanguageId::Cpp) {
        return;
    }

    static const QRegularExpression wordRegex(QStringLiteral("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\b"));
    auto it = wordRegex.globalMatch(text);
    while (it.hasNext()) {
        const auto match = it.next();
        if (cppKeywords().contains(match.captured(1))) {
            setFormat(match.capturedStart(1), match.capturedLength(1), m_keywordFormat);
        }
    }
}
