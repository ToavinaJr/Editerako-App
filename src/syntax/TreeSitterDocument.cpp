#include "syntax/TreeSitterDocument.h"

#include "core/Logging.h"

#include <QTextDocument>

namespace {

struct TextIndex {
    uint32_t byte = 0;
    TSPoint point{0, 0};
};

TextIndex indexAtUtf16(const QString &text, int utf16Pos)
{
    TextIndex idx;
    const int n = qMin(utf16Pos, static_cast<int>(text.size()));
    int i = 0;
    while (i < n) {
        int units = 1;
        if (text[i].isHighSurrogate() && (i + 1) < text.size() && text[i + 1].isLowSurrogate()) {
            units = 2;
        }
        const QByteArray u8 = QStringView(text).mid(i, units).toUtf8();
        const auto nbytes = static_cast<uint32_t>(u8.size());
        if (units == 1 && text[i] == QLatin1Char('\n')) {
            idx.point.row += 1;
            idx.point.column = 0;
        } else {
            idx.point.column += nbytes;
        }
        idx.byte += nbytes;
        i += units;
    }
    return idx;
}

} // namespace

TreeSitterDocument::TreeSitterDocument(QTextDocument *document, LanguageId language, QObject *parent)
    : QObject(parent)
    , m_document(document)
    , m_language(language)
{
    Q_ASSERT(document);

    m_parser = ts_parser_new();
    const TSLanguage *tsLang = LanguageRegistry::tsLanguage(language);
    if (!m_parser || !tsLang || !ts_parser_set_language(m_parser, tsLang)) {
        qCWarning(lcSyntax) << "Tree-sitter parser setup failed for"
                            << LanguageRegistry::displayName(language);
        if (m_parser) {
            ts_parser_delete(m_parser);
            m_parser = nullptr;
        }
        return;
    }

    m_text = document->toPlainText();
    parseFull();
    connect(document, &QTextDocument::contentsChange, this, &TreeSitterDocument::onContentsChange);
}

TreeSitterDocument::~TreeSitterDocument()
{
    if (m_tree) {
        ts_tree_delete(m_tree);
        m_tree = nullptr;
    }
    if (m_parser) {
        ts_parser_delete(m_parser);
        m_parser = nullptr;
    }
}

bool TreeSitterDocument::isReady() const
{
    return m_parser && m_tree;
}

void TreeSitterDocument::rebuildLineStarts()
{
    m_lineStartBytes.clear();
    m_lineStartBytes.push_back(0);
    const char *data = m_utf8.constData();
    const int n = static_cast<int>(m_utf8.size());
    for (int i = 0; i < n; ++i) {
        if (data[i] == '\n') {
            m_lineStartBytes.push_back(static_cast<uint32_t>(i + 1));
        }
    }
}

void TreeSitterDocument::parseFull()
{
    m_utf8 = m_text.toUtf8();
    rebuildLineStarts();
    if (!m_parser) {
        return;
    }

    TSTree *parsed = ts_parser_parse_string(
        m_parser, nullptr, m_utf8.constData(), static_cast<uint32_t>(m_utf8.size()));
    if (m_tree) {
        ts_tree_delete(m_tree);
    }
    m_tree = parsed;
}

void TreeSitterDocument::onContentsChange(int position, int charsRemoved, int charsAdded)
{
    if (!m_parser) {
        return;
    }
    if (charsRemoved == 0 && charsAdded == 0) {
        return;
    }

    const TextIndex start = indexAtUtf16(m_text, position);
    const TextIndex oldEnd = indexAtUtf16(m_text, position + charsRemoved);

    m_text = m_document->toPlainText();
    const TextIndex newEnd = indexAtUtf16(m_text, position + charsAdded);

    if (m_tree) {
        TSInputEdit edit{};
        edit.start_byte = start.byte;
        edit.old_end_byte = oldEnd.byte;
        edit.new_end_byte = newEnd.byte;
        edit.start_point = start.point;
        edit.old_end_point = oldEnd.point;
        edit.new_end_point = newEnd.point;
        ts_tree_edit(m_tree, &edit);
    }

    m_utf8 = m_text.toUtf8();
    rebuildLineStarts();

    TSTree *parsed = ts_parser_parse_string(
        m_parser, m_tree, m_utf8.constData(), static_cast<uint32_t>(m_utf8.size()));
    if (m_tree) {
        ts_tree_delete(m_tree);
    }
    m_tree = parsed;
}

bool TreeSitterDocument::utf8RangeForBlock(int blockNumber, uint32_t *start, uint32_t *end) const
{
    if (!start || !end || blockNumber < 0 || blockNumber >= m_lineStartBytes.size()) {
        return false;
    }

    *start = m_lineStartBytes.at(blockNumber);
    if (blockNumber + 1 < m_lineStartBytes.size()) {
        *end = m_lineStartBytes.at(blockNumber + 1);
        if (*end > *start) {
            *end -= 1;
        }
    } else {
        *end = static_cast<uint32_t>(m_utf8.size());
    }
    return true;
}

void TreeSitterDocument::visitOverlapping(uint32_t startByte, uint32_t endByte,
                                          const std::function<void(TSNode)> &visitor) const
{
    if (!m_tree || startByte >= endByte || !visitor) {
        return;
    }

    const std::function<void(TSNode)> walk = [&](TSNode node) {
        const uint32_t nodeStart = ts_node_start_byte(node);
        const uint32_t nodeEnd = ts_node_end_byte(node);
        if (nodeEnd <= startByte || nodeStart >= endByte) {
            return;
        }
        visitor(node);
        const uint32_t n = ts_node_child_count(node);
        for (uint32_t i = 0; i < n; ++i) {
            walk(ts_node_child(node, i));
        }
    };
    walk(ts_tree_root_node(m_tree));
}
