#include "editor/features/IndentOps.h"

#include <QtTest>

class IndentOpsTest : public QObject
{
    Q_OBJECT

private slots:
    void indentAndOutdent();
    void smartIndent();
    void convertAndTrimAndSort();
};

void IndentOpsTest::indentAndOutdent()
{
    QStringList lines{QStringLiteral("a"), QStringLiteral("b")};
    indentLines(&lines, 4, true);
    QCOMPARE(lines, QStringList({QStringLiteral("    a"), QStringLiteral("    b")}));
    outdentLines(&lines, 4, true);
    QCOMPARE(lines, QStringList({QStringLiteral("a"), QStringLiteral("b")}));

    QStringList tabs{QStringLiteral("\tx")};
    outdentLines(&tabs, 4, false);
    QCOMPARE(tabs, QStringList({QStringLiteral("x")}));
}

void IndentOpsTest::smartIndent()
{
    QCOMPARE(smartIndentPrefix(QStringLiteral("    if (x) {"), 4, true),
             QStringLiteral("        "));
    QCOMPARE(smartIndentPrefix(QStringLiteral("value:"), 2, true), QStringLiteral("  "));
    QCOMPARE(smartIndentPrefix(QStringLiteral("  foo"), 4, true), QStringLiteral("  "));
}

void IndentOpsTest::convertAndTrimAndSort()
{
    QCOMPARE(convertIndentation(QStringLiteral("\ta"), true, 4), QStringLiteral("    a"));
    QCOMPARE(convertIndentation(QStringLiteral("    a"), false, 4), QStringLiteral("\ta"));
    QCOMPARE(trimTrailingWhitespace(QStringLiteral("ab  \ncd\t")), QStringLiteral("ab\ncd"));
    QCOMPARE(sortLinesText(QStringLiteral("b\na\nc")), QStringLiteral("a\nb\nc"));
}

QTEST_GUILESS_MAIN(IndentOpsTest)
#include "IndentOpsTest.moc"
