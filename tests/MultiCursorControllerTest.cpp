#include "editor/features/MultiCursorController.h"

#include <QPlainTextEdit>
#include <QTextCursor>
#include <QtTest>

class MultiCursorControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void toggleAddsAndRemoves();
    void normalizeDropsPrimaryDuplicate();
    void insertTextAtAllCursors();
};

void MultiCursorControllerTest::toggleAddsAndRemoves()
{
    QPlainTextEdit edit;
    edit.setPlainText(QStringLiteral("abcdef"));
    QTextCursor primary = edit.textCursor();
    primary.setPosition(5);
    edit.setTextCursor(primary);

    MultiCursorController multi(&edit);
    QTextCursor extra(edit.document());
    extra.setPosition(0);
    multi.toggleAt(extra);
    QCOMPARE(multi.extraCount(), 1);

    multi.toggleAt(extra);
    QCOMPARE(multi.extraCount(), 0);
}

void MultiCursorControllerTest::normalizeDropsPrimaryDuplicate()
{
    QPlainTextEdit edit;
    edit.setPlainText(QStringLiteral("abc"));
    QTextCursor primary = edit.textCursor();
    primary.setPosition(0);
    edit.setTextCursor(primary);

    MultiCursorController multi(&edit);
    QTextCursor same(edit.document());
    same.setPosition(0);
    multi.toggleAt(same);
    QCOMPARE(multi.extraCount(), 0);
}

void MultiCursorControllerTest::insertTextAtAllCursors()
{
    QPlainTextEdit edit;
    edit.setPlainText(QStringLiteral("abc"));
    QTextCursor primary = edit.textCursor();
    primary.setPosition(3);
    edit.setTextCursor(primary);

    MultiCursorController multi(&edit);
    QTextCursor extra(edit.document());
    extra.setPosition(0);
    multi.toggleAt(extra);
    QCOMPARE(multi.extraCount(), 1);

    multi.insertText(QStringLiteral("X"));
    QCOMPARE(edit.toPlainText(), QStringLiteral("XabcX"));
}

QTEST_MAIN(MultiCursorControllerTest)
#include "MultiCursorControllerTest.moc"
