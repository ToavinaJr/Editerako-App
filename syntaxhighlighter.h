#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QColor>
#include <tree_sitter/api.h>
#include "codeeditor.h"

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    enum Language { CPP, HTML };

    SyntaxHighlighter(CodeEditor *editor, Language lang);
    ~SyntaxHighlighter();

protected:
    void highlightBlock(const QString &text) override;

private:
    Language language;
    TSParser *parser;
    TSTree *tree;

    void highlightCpp(const QString &text);
    void highlightHtml(const QString &text);

    void setupFormats();

    QTextCharFormat keywordFormat;
    QTextCharFormat typeFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat preprocFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat variableFormat;
    QTextCharFormat parameterFormat;
    QTextCharFormat punctuationFormat;
    QTextCharFormat operatorFormat;
    QTextCharFormat namespaceFormat; // Ajouté
};

#endif // SYNTAXHIGHLIGHTER_H
