#include "editor/CompletionModel.h"

#include <QPlainTextEdit>
#include <QTextCursor>
#include <QtTest>

class CompletionModelTest : public QObject
{
    Q_OBJECT

private slots:
    void filtersAndSortsBySortText();
    void matchesFilterText();
    void applyInsertsLabelWhenNoInsertText();
    void applyUsesTextEditRange();
};

void CompletionModelTest::filtersAndSortsBySortText()
{
    CompletionModel model;
    CompletionItem a;
    a.label = QStringLiteral("zeta");
    a.sortText = QStringLiteral("2");
    CompletionItem b;
    b.label = QStringLiteral("alpha");
    b.sortText = QStringLiteral("1");
    CompletionItem c;
    c.label = QStringLiteral("beta");
    c.sortText = QStringLiteral("3");
    model.setItems({a, b, c});
    QCOMPARE(model.visibleCount(), 3);
    QCOMPARE(model.itemAt(0).label, QStringLiteral("alpha"));

    model.setFilter(QStringLiteral("be"));
    QCOMPARE(model.visibleCount(), 1);
    QCOMPARE(model.itemAt(0).label, QStringLiteral("beta"));
}

void CompletionModelTest::matchesFilterText()
{
    CompletionModel model;
    CompletionItem item;
    item.label = QStringLiteral("insert");
    item.filterText = QStringLiteral("sender");
    model.setItems({item});
    model.setFilter(QStringLiteral("sen"));
    QCOMPARE(model.visibleCount(), 1);
    QCOMPARE(model.itemAt(0).label, QStringLiteral("insert"));
}

void CompletionModelTest::applyInsertsLabelWhenNoInsertText()
{
    QPlainTextEdit editor;
    editor.setPlainText(QStringLiteral("int fo"));
    QTextCursor cursor = editor.textCursor();
    cursor.movePosition(QTextCursor::End);
    editor.setTextCursor(cursor);

    CompletionItem item;
    item.label = QStringLiteral("foo");
    applyCompletionItem(&editor, item);
    QCOMPARE(editor.toPlainText(), QStringLiteral("int foo"));
}

void CompletionModelTest::applyUsesTextEditRange()
{
    QPlainTextEdit editor;
    editor.setPlainText(QStringLiteral("abc"));
    CompletionItem item;
    item.insertText = QStringLiteral("xyz");
    item.hasTextEdit = true;
    item.endCharacter = 3;
    applyCompletionItem(&editor, item);
    QCOMPARE(editor.toPlainText(), QStringLiteral("xyz"));
}

QTEST_MAIN(CompletionModelTest)
#include "CompletionModelTest.moc"
