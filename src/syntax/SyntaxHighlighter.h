#ifndef EDITERAKO_SYNTAXHIGHLIGHTER_H
#define EDITERAKO_SYNTAXHIGHLIGHTER_H

#include "syntax/LanguageRegistry.h"

#include <QHash>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <memory>

class HighlightQuery;
class TreeSitterDocument;
class QTextDocument;

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    SyntaxHighlighter(QTextDocument *document, LanguageId language);
    ~SyntaxHighlighter() override;

    [[nodiscard]] LanguageId language() const { return m_language; }

protected:
    void highlightBlock(const QString &text) override;

private:
    void setupFormats();
    [[nodiscard]] QTextCharFormat formatForCapture(const QString &captureName) const;

    LanguageId m_language = LanguageId::PlainText;
    TreeSitterDocument *m_treeDocument = nullptr;
    std::unique_ptr<HighlightQuery> m_query;
    QHash<QString, QTextCharFormat> m_formats;
};

#endif
