#include "editor/features/LineMovementController.h"

#include <QPlainTextEdit>
#include <QTextCursor>
#include <QtTest>

class LineMovementControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void moveUpSwapsWithPrevious();
    void moveUpAtFirstLineIsNoOp();
    void moveDownSwapsWithNext();
    void moveDownAtLastLineIsNoOp();
};

void LineMovementControllerTest::moveUpSwapsWithPrevious()
{
    QPlainTextEdit edit;
    edit.setPlainText(QStringLiteral("a\nb\nc"));
    QTextCursor cursor = edit.textCursor();
    cursor.setPosition(2);
    edit.setTextCursor(cursor);

    LineMovementController movement(&edit);
    movement.moveUp();
    QCOMPARE(edit.toPlainText(), QStringLiteral("b\na\nc"));
}

void LineMovementControllerTest::moveUpAtFirstLineIsNoOp()
{
    QPlainTextEdit edit;
    edit.setPlainText(QStringLiteral("a\nb\nc"));
    const QString before = edit.toPlainText();

    LineMovementController movement(&edit);
    movement.moveUp();
    QCOMPARE(edit.toPlainText(), before);
}

void LineMovementControllerTest::moveDownSwapsWithNext()
{
    QPlainTextEdit edit;
    edit.setPlainText(QStringLiteral("a\nb\nc"));
    QTextCursor cursor = edit.textCursor();
    cursor.setPosition(0);
    edit.setTextCursor(cursor);

    LineMovementController movement(&edit);
    movement.moveDown();
    QCOMPARE(edit.toPlainText(), QStringLiteral("b\na\nc"));
}

void LineMovementControllerTest::moveDownAtLastLineIsNoOp()
{
    QPlainTextEdit edit;
    edit.setPlainText(QStringLiteral("a\nb\nc"));
    QTextCursor cursor = edit.textCursor();
    cursor.movePosition(QTextCursor::End);
    edit.setTextCursor(cursor);
    const QString before = edit.toPlainText();

    LineMovementController movement(&edit);
    movement.moveDown();
    QCOMPARE(edit.toPlainText(), before);
}

QTEST_MAIN(LineMovementControllerTest)
#include "LineMovementControllerTest.moc"
