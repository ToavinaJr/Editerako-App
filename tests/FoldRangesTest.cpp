#include "syntax/FoldRanges.h"
#include "syntax/TreeSitterDocument.h"

#include <QPlainTextDocumentLayout>
#include <QTextDocument>
#include <QtTest>

class FoldRangesTest : public QObject
{
    Q_OBJECT

private:
    static void attachLayout(QTextDocument *document)
    {
        document->setDocumentLayout(new QPlainTextDocumentLayout(document));
    }

    static bool hasRangeStartingAt(const QVector<FoldRange> &ranges, int startLine)
    {
        for (const FoldRange &range : ranges) {
            if (range.startLine == startLine && range.endLine > startLine) {
                return true;
            }
        }
        return false;
    }

private slots:
    void cppFunctionIsFoldable();
    void jsonObjectIsFoldable();
    void singleLineHasNoFold();
    void plainTextHasNoTree();
};

void FoldRangesTest::cppFunctionIsFoldable()
{
    QTextDocument document;
    attachLayout(&document);
    document.setPlainText(QStringLiteral("int main() {\n    return 0;\n}\n"));
    TreeSitterDocument tree(&document, LanguageId::Cpp);
    QVERIFY(tree.isReady());

    const QVector<FoldRange> ranges = foldRangesFromTree(tree.rootNode());
    QVERIFY(!ranges.isEmpty());
    QVERIFY(hasRangeStartingAt(ranges, 0));
}

void FoldRangesTest::jsonObjectIsFoldable()
{
    QTextDocument document;
    attachLayout(&document);
    document.setPlainText(QStringLiteral("{\n  \"a\": 1\n}\n"));
    TreeSitterDocument tree(&document, LanguageId::Json);
    QVERIFY(tree.isReady());

    const QVector<FoldRange> ranges = foldRangesFromTree(tree.rootNode());
    QVERIFY(!ranges.isEmpty());
}

void FoldRangesTest::singleLineHasNoFold()
{
    QTextDocument document;
    attachLayout(&document);
    document.setPlainText(QStringLiteral("int x = 1;"));
    TreeSitterDocument tree(&document, LanguageId::Cpp);
    QVERIFY(tree.isReady());
    QVERIFY(foldRangesFromTree(tree.rootNode()).isEmpty());
}

void FoldRangesTest::plainTextHasNoTree()
{
    QTextDocument document;
    attachLayout(&document);
    document.setPlainText(QStringLiteral("hello\nworld\n"));
    TreeSitterDocument tree(&document, LanguageId::PlainText);
    QVERIFY(!tree.isReady());
}

QTEST_MAIN(FoldRangesTest)
#include "FoldRangesTest.moc"
