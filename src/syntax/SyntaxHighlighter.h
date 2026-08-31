#ifndef EDITERAKO_SYNTAXHIGHLIGHTER_H
#define EDITERAKO_SYNTAXHIGHLIGHTER_H

#include "syntax/LanguageRegistry.h"

#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class TreeSitterDocument;
class QTextDocument;

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    SyntaxHighlighter(QTextDocument *document, LanguageId language);
    ~SyntaxHighlighter() override = default;

    [[nodiscard]] LanguageId language() const { return m_language; }

protected:
    void highlightBlock(const QString &text) override;

private:
    void setupFormats();
    void applyNodeFormat(const char *type, int start, int length);

    LanguageId m_language = LanguageId::PlainText;
    TreeSitterDocument *m_treeDocument = nullptr;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_typeFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_preprocFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_variableFormat;
    QTextCharFormat m_parameterFormat;
    QTextCharFormat m_punctuationFormat;
    QTextCharFormat m_operatorFormat;
    QTextCharFormat m_namespaceFormat;
};

#endif
