#include "editor/features/LineEditCommands.h"
#include "editor/features/OccurrenceController.h"
#include "editor/features/MultiCursorController.h"

#include <QPlainTextEdit>
#include <QTextCursor>
#include <QtTest>

class LineEditCommandsTest : public QObject
{
    Q_OBJECT

private slots:
    void duplicateAndDelete();
    void joinAndSort();
    void selectNextOccurrence();
};

void LineEditCommandsTest::duplicateAndDelete()
{
    QPlainTextEdit edit;
    edit.setPlainText(QStringLiteral("a\nb\nc"));
    QTextCursor cursor = edit.textCursor();
    cursor.setPosition(0);
    edit.setTextCursor(cursor);

    LineEditCommands commands(&edit);
    commands.duplicateLine();
    QCOMPARE(edit.toPlainText(), QStringLiteral("a\na\nb\nc"));
    commands.deleteLine();
    QVERIFY(edit.toPlainText().startsWith(QStringLiteral("a")));
}

void LineEditCommandsTest::joinAndSort()
{
    QPlainTextEdit edit;
    edit.setPlainText(QStringLiteral("b\na\nc"));
    QTextCursor cursor = edit.textCursor();
    cursor.setPosition(0);
    cursor.setPosition(edit.toPlainText().size(), QTextCursor::KeepAnchor);
    edit.setTextCursor(cursor);

    LineEditCommands commands(&edit);
    commands.sortLines();
    QCOMPARE(edit.toPlainText(), QStringLiteral("a\nb\nc"));

    cursor.setPosition(0);
    cursor.setPosition(3, QTextCursor::KeepAnchor);
    edit.setTextCursor(cursor);
    commands.joinLines();
    QCOMPARE(edit.toPlainText(), QStringLiteral("a b\nc"));
}

void LineEditCommandsTest::selectNextOccurrence()
{
    QPlainTextEdit edit;
    edit.setPlainText(QStringLiteral("foo bar foo"));
    QTextCursor cursor = edit.textCursor();
    cursor.setPosition(0);
    cursor.setPosition(3, QTextCursor::KeepAnchor);
    edit.setTextCursor(cursor);

    MultiCursorController multi(&edit);
    OccurrenceController occurrences(&edit, &multi);
    occurrences.selectNext();
    QCOMPARE(multi.extraCount(), 1);
    QCOMPARE(edit.textCursor().selectedText(), QStringLiteral("foo"));
}

QTEST_MAIN(LineEditCommandsTest)
#include "LineEditCommandsTest.moc"
