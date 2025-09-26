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

    QTextCharFormat keywordFormat;
    QTextCharFormat typeFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat commentFormat;

    void setupFormats();
};

#endif // SYNTAXHIGHLIGHTER_H
