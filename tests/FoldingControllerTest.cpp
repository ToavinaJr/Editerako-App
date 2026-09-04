#include "editor/CodeEditor.h"
#include "editor/EditorDocument.h"
#include "editor/HighlighterSync.h"

#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QtTest>

class FoldingControllerTest : public QObject
{
    Q_OBJECT

private:
    static void attachCpp(CodeEditor *editor)
    {
        auto *doc = new EditorDocument(editor, editor);
        doc->setFilePath(QStringLiteral("snippet.cpp"));
        editor->setPlainText(QStringLiteral("int main() {\n    return 0;\n}\n"));
        HighlighterSync::apply(editor);
        editor->refreshFolds();
    }

    static int visibleBlockCount(const QTextDocument *document)
    {
        int n = 0;
        for (QTextBlock block = document->firstBlock(); block.isValid(); block = block.next()) {
            if (block.isVisible()) {
                ++n;
            }
        }
        return n;
    }

private slots:
    void foldAllHidesInnerLines();
    void unfoldAllRestoresLines();
    void toggleFoldAtStart();
};

void FoldingControllerTest::foldAllHidesInnerLines()
{
    CodeEditor editor;
    attachCpp(&editor);

    QVERIFY(editor.document()->blockCount() >= 3);
    editor.foldAll();
    QVERIFY(visibleBlockCount(editor.document()) < editor.document()->blockCount());
    QVERIFY(!editor.document()->findBlockByNumber(1).isVisible());
}

void FoldingControllerTest::unfoldAllRestoresLines()
{
    CodeEditor editor;
    attachCpp(&editor);
    editor.foldAll();
    editor.unfoldAll();
    QCOMPARE(visibleBlockCount(editor.document()), editor.document()->blockCount());
}

void FoldingControllerTest::toggleFoldAtStart()
{
    CodeEditor editor;
    attachCpp(&editor);

    QTextCursor cursor = editor.textCursor();
    cursor.movePosition(QTextCursor::Start);
    editor.setTextCursor(cursor);
    editor.toggleFold();
    QVERIFY(!editor.document()->findBlockByNumber(1).isVisible());
    editor.toggleFold();
    QVERIFY(editor.document()->findBlockByNumber(1).isVisible());
}

QTEST_MAIN(FoldingControllerTest)
#include "FoldingControllerTest.moc"
