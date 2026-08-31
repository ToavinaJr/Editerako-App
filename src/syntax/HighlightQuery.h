#ifndef EDITERAKO_HIGHLIGHTQUERY_H
#define EDITERAKO_HIGHLIGHTQUERY_H

#include <QByteArray>
#include <QString>
#include <QVector>

#include <tree_sitter/api.h>

class HighlightQuery
{
public:
    struct Capture {
        QString name;
        uint32_t startByte = 0;
        uint32_t endByte = 0;
    };

    HighlightQuery(const TSLanguage *language, const QByteArray &source);
    ~HighlightQuery();

    HighlightQuery(const HighlightQuery &) = delete;
    HighlightQuery &operator=(const HighlightQuery &) = delete;

    [[nodiscard]] bool isValid() const { return m_query != nullptr; }
    [[nodiscard]] QString errorString() const { return m_error; }

    [[nodiscard]] QVector<Capture> captures(TSNode root, uint32_t startByte, uint32_t endByte,
                                            const QByteArray &utf8) const;

private:
    [[nodiscard]] bool patternPassesPredicates(const TSQueryMatch &match,
                                               const QByteArray &utf8) const;
    [[nodiscard]] QByteArray captureText(const TSQueryMatch &match, uint32_t captureId,
                                         const QByteArray &utf8) const;

    TSQuery *m_query = nullptr;
    mutable TSQueryCursor *m_cursor = nullptr;
    QString m_error;
};

#endif
