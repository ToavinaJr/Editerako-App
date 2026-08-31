#include "syntax/TreeSitterDocument.h"

#include <QPlainTextDocumentLayout>
#include <QTextCursor>
#include <QTextDocument>
#include <QtTest>

class TreeSitterDocumentTest : public QObject
{
    Q_OBJECT

private:
    static void attachEditorLayout(QTextDocument *document)
    {
        document->setDocumentLayout(new QPlainTextDocumentLayout(document));
    }

    static int nodeCountInRange(const TreeSitterDocument &doc, uint32_t start, uint32_t end)
    {
        int n = 0;
        doc.visitOverlapping(start, end, [&](TSNode) { ++n; });
        return n;
    }

private slots:
    void parsesCppAscii();
    void insertDeleteReplace();
    void unicodeAndEmoji();
    void multilineAndRawString();
    void htmlMultilineComment();
    void plainTextHasNoParser();
};

void TreeSitterDocumentTest::parsesCppAscii()
{
    QTextDocument document;
    attachEditorLayout(&document);
    document.setPlainText(QStringLiteral("int main() { return 0; }"));
    TreeSitterDocument tree(&document, LanguageId::Cpp);
    QVERIFY(tree.isReady());

    uint32_t start = 0;
    uint32_t end = 0;
    QVERIFY(tree.utf8RangeForBlock(0, &start, &end));
    QVERIFY(end > start);
    QVERIFY(nodeCountInRange(tree, start, end) > 1);
}

void TreeSitterDocumentTest::insertDeleteReplace()
{
    QTextDocument document;
    attachEditorLayout(&document);
    document.setPlainText(QStringLiteral("int x = 1;"));
    TreeSitterDocument tree(&document, LanguageId::Cpp);
    QVERIFY(tree.isReady());

    QTextCursor cursor(&document);
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(QStringLiteral("\nint y = 2;"));
    QVERIFY(tree.isReady());

    cursor = QTextCursor(&document);
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    QVERIFY(tree.isReady());

    cursor = QTextCursor(&document);
    cursor.select(QTextCursor::Document);
    cursor.insertText(QStringLiteral("void f() {}"));
    QVERIFY(tree.isReady());

    uint32_t start = 0;
    uint32_t end = 0;
    QVERIFY(tree.utf8RangeForBlock(0, &start, &end));
    QVERIFY(nodeCountInRange(tree, start, end) > 0);
}

void TreeSitterDocumentTest::unicodeAndEmoji()
{
    QTextDocument document;
    attachEditorLayout(&document);
    document.setPlainText(QStringLiteral("const char *s = \"hello\";"));
    TreeSitterDocument tree(&document, LanguageId::Cpp);
    QVERIFY(tree.isReady());

    const int quote = document.toPlainText().indexOf(QLatin1Char('"'));
    QVERIFY(quote >= 0);
    QTextCursor cursor(&document);
    cursor.setPosition(quote + 1);
    cursor.insertText(QStringLiteral("café你好🚀"));
    QVERIFY(tree.isReady());

    uint32_t start = 0;
    uint32_t end = 0;
    QVERIFY(tree.utf8RangeForBlock(0, &start, &end));
    QVERIFY(end > start);
}

void TreeSitterDocumentTest::multilineAndRawString()
{
    QTextDocument document;
    attachEditorLayout(&document);
    document.setPlainText(QStringLiteral("int a = 1;"));
    TreeSitterDocument tree(&document, LanguageId::Cpp);
    QVERIFY(tree.isReady());

    QTextCursor cursor(&document);
    cursor.select(QTextCursor::Document);
    cursor.insertText(QStringLiteral("const char *r = R\"(\nline1\nline2)\";\n/*\ncomment\n*/\n"));
    QVERIFY(tree.isReady());
    QVERIFY(document.blockCount() >= 3);

    uint32_t start = 0;
    uint32_t end = 0;
    QVERIFY(tree.utf8RangeForBlock(0, &start, &end));
    QVERIFY(tree.utf8RangeForBlock(1, &start, &end));
    QVERIFY(tree.utf8RangeForBlock(2, &start, &end));
}

void TreeSitterDocumentTest::htmlMultilineComment()
{
    QTextDocument document;
    attachEditorLayout(&document);
    document.setPlainText(QStringLiteral("<div>ok</div>"));
    TreeSitterDocument tree(&document, LanguageId::Html);
    QVERIFY(tree.isReady());

    QTextCursor cursor(&document);
    cursor.movePosition(QTextCursor::Start);
    cursor.insertText(QStringLiteral("<!--\nhello\n-->\n"));
    QVERIFY(tree.isReady());

    uint32_t start = 0;
    uint32_t end = 0;
    QVERIFY(tree.utf8RangeForBlock(0, &start, &end));
}

void TreeSitterDocumentTest::plainTextHasNoParser()
{
    QTextDocument document;
    attachEditorLayout(&document);
    document.setPlainText(QStringLiteral("hello"));
    TreeSitterDocument tree(&document, LanguageId::PlainText);
    QVERIFY(!tree.isReady());
}

QTEST_MAIN(TreeSitterDocumentTest)
#include "TreeSitterDocumentTest.moc"
