#include "syntax/HighlightQuery.h"

#include <QRegularExpression>

namespace {

QByteArray nodeUtf8(TSNode node, const QByteArray &utf8)
{
    if (ts_node_is_null(node)) {
        return {};
    }
    const uint32_t start = ts_node_start_byte(node);
    const uint32_t end = ts_node_end_byte(node);
    if (end < start || end > static_cast<uint32_t>(utf8.size())) {
        return {};
    }
    return utf8.mid(static_cast<int>(start), static_cast<int>(end - start));
}

const char *queryErrorName(TSQueryError error)
{
    switch (error) {
    case TSQueryErrorNone:
        return "none";
    case TSQueryErrorSyntax:
        return "syntax";
    case TSQueryErrorNodeType:
        return "node type";
    case TSQueryErrorField:
        return "field";
    case TSQueryErrorCapture:
        return "capture";
    case TSQueryErrorStructure:
        return "structure";
    case TSQueryErrorLanguage:
        return "language";
    }
    return "unknown";
}

} // namespace

HighlightQuery::HighlightQuery(const TSLanguage *language, const QByteArray &source)
{
    if (!language || source.isEmpty()) {
        m_error = QStringLiteral("Empty language or query source");
        return;
    }

    uint32_t errorOffset = 0;
    TSQueryError errorType = TSQueryErrorNone;
    m_query = ts_query_new(language, source.constData(), static_cast<uint32_t>(source.size()),
                           &errorOffset, &errorType);
    if (!m_query) {
        m_error = QStringLiteral("Query %1 error at byte %2")
                      .arg(QLatin1String(queryErrorName(errorType)))
                      .arg(errorOffset);
        return;
    }

    m_cursor = ts_query_cursor_new();
    if (!m_cursor) {
        ts_query_delete(m_query);
        m_query = nullptr;
        m_error = QStringLiteral("Failed to create query cursor");
    }
}

HighlightQuery::~HighlightQuery()
{
    if (m_cursor) {
        ts_query_cursor_delete(m_cursor);
        m_cursor = nullptr;
    }
    if (m_query) {
        ts_query_delete(m_query);
        m_query = nullptr;
    }
}

QByteArray HighlightQuery::captureText(const TSQueryMatch &match, uint32_t captureId,
                                       const QByteArray &utf8) const
{
    for (uint16_t i = 0; i < match.capture_count; ++i) {
        if (match.captures[i].index == captureId) {
            return nodeUtf8(match.captures[i].node, utf8);
        }
    }
    return {};
}

bool HighlightQuery::patternPassesPredicates(const TSQueryMatch &match, const QByteArray &utf8) const
{
    uint32_t stepCount = 0;
    const TSQueryPredicateStep *steps =
        ts_query_predicates_for_pattern(m_query, match.pattern_index, &stepCount);
    if (!steps || stepCount == 0) {
        return true;
    }

    uint32_t i = 0;
    while (i < stepCount) {
        if (steps[i].type != TSQueryPredicateStepTypeString) {
            while (i < stepCount && steps[i].type != TSQueryPredicateStepTypeDone) {
                ++i;
            }
            if (i < stepCount) {
                ++i;
            }
            continue;
        }

        uint32_t nameLen = 0;
        const char *rawName = ts_query_string_value_for_id(m_query, steps[i].value_id, &nameLen);
        const QByteArray name = QByteArray::fromRawData(rawName, static_cast<int>(nameLen));
        ++i;

        QVector<TSQueryPredicateStep> args;
        while (i < stepCount && steps[i].type != TSQueryPredicateStepTypeDone) {
            args.append(steps[i]);
            ++i;
        }
        if (i < stepCount) {
            ++i;
        }

        auto argText = [&](const TSQueryPredicateStep &step) -> QByteArray {
            if (step.type == TSQueryPredicateStepTypeCapture) {
                return captureText(match, step.value_id, utf8);
            }
            if (step.type == TSQueryPredicateStepTypeString) {
                uint32_t len = 0;
                const char *s = ts_query_string_value_for_id(m_query, step.value_id, &len);
                return QByteArray(s, static_cast<int>(len));
            }
            return {};
        };

        if (name == "eq?" || name == "not-eq?") {
            if (args.size() < 2) {
                return false;
            }
            const bool eq = argText(args.at(0)) == argText(args.at(1));
            if ((name == "eq?") != eq) {
                return false;
            }
        } else if (name == "match?" || name == "not-match?") {
            if (args.size() < 2) {
                return false;
            }
            const QRegularExpression re(QString::fromUtf8(argText(args.at(1))));
            if (!re.isValid()) {
                return false;
            }
            const bool ok = re.match(QString::fromUtf8(argText(args.at(0)))).hasMatch();
            if ((name == "match?") != ok) {
                return false;
            }
        } else if (name == "any-of?") {
            if (args.size() < 2) {
                return false;
            }
            const QByteArray value = argText(args.at(0));
            bool found = false;
            for (int a = 1; a < args.size(); ++a) {
                if (value == argText(args.at(a))) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
        // Directives such as #set! are ignored.
    }
    return true;
}

QVector<HighlightQuery::Capture> HighlightQuery::captures(TSNode root, uint32_t startByte,
                                                          uint32_t endByte,
                                                          const QByteArray &utf8) const
{
    QVector<Capture> result;
    if (!m_query || !m_cursor || ts_node_is_null(root) || startByte >= endByte) {
        return result;
    }

    ts_query_cursor_set_byte_range(m_cursor, startByte, endByte);
    ts_query_cursor_exec(m_cursor, m_query, root);

    TSQueryMatch match{};
    uint32_t captureIndex = 0;
    while (ts_query_cursor_next_capture(m_cursor, &match, &captureIndex)) {
        if (!patternPassesPredicates(match, utf8)) {
            ts_query_cursor_remove_match(m_cursor, match.id);
            continue;
        }
        if (captureIndex >= match.capture_count) {
            continue;
        }
        const TSQueryCapture cap = match.captures[captureIndex];
        const uint32_t capStart = ts_node_start_byte(cap.node);
        const uint32_t capEnd = ts_node_end_byte(cap.node);
        if (capEnd <= startByte || capStart >= endByte || capEnd <= capStart) {
            continue;
        }

        uint32_t nameLen = 0;
        const char *name = ts_query_capture_name_for_id(m_query, cap.index, &nameLen);
        Capture out;
        out.name = QString::fromUtf8(name, static_cast<int>(nameLen));
        out.startByte = qMax(capStart, startByte);
        out.endByte = qMin(capEnd, endByte);
        result.append(out);
    }
    return result;
}
