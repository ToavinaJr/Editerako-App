#include "editor/features/CommentOps.h"

#include <QtTest>

class CommentOpsTest : public QObject
{
    Q_OBJECT

private slots:
    void toggleLine();
    void toggleBlock();
};

void CommentOpsTest::toggleLine()
{
    const QStringList commented = toggleLineComments({QStringLiteral("int x;")}, QStringLiteral("//"));
    QCOMPARE(commented, QStringList({QStringLiteral("// int x;")}));
    QCOMPARE(toggleLineComments(commented, QStringLiteral("//")),
             QStringList({QStringLiteral("int x;")}));
}

void CommentOpsTest::toggleBlock()
{
    QCOMPARE(toggleBlockComment(QStringLiteral("x"), QStringLiteral("/*"), QStringLiteral("*/")),
             QStringLiteral("/*x*/"));
    QCOMPARE(toggleBlockComment(QStringLiteral("/*x*/"), QStringLiteral("/*"), QStringLiteral("*/")),
             QStringLiteral("x"));
}

QTEST_GUILESS_MAIN(CommentOpsTest)
#include "CommentOpsTest.moc"
