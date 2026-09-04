#include "tasks/ProblemMatcher.h"

#include <QtTest>

class ProblemMatcherTest : public QObject
{
    Q_OBJECT

private slots:
    void matchesGcc();
    void matchesMsvc();
    void ignoresUnknownMatcher();
};

void ProblemMatcherTest::matchesGcc()
{
    const QString output = QStringLiteral(
        "src/app/MainWindow.cpp:12:5: error: 'foo' was not declared in this scope\n"
        "src/app/MainWindow.cpp:20:1: warning: unused variable 'x'\n"
        "ninja: build stopped\n");
    const QVector<TaskProblem> problems = matchTaskProblems(
        output, QStringLiteral("gcc"), QStringLiteral("C:/proj"), QStringLiteral("C:/proj"));
    QCOMPARE(problems.size(), 2);
    QCOMPARE(problems.at(0).line, 12);
    QCOMPARE(problems.at(0).column, 5);
    QCOMPARE(problems.at(0).severity, TaskProblem::Severity::Error);
    QVERIFY(problems.at(0).path.endsWith(QStringLiteral("src/app/MainWindow.cpp")));
    QCOMPARE(problems.at(1).severity, TaskProblem::Severity::Warning);
}

void ProblemMatcherTest::matchesMsvc()
{
    const QString output =
        QStringLiteral("C:\\proj\\a.cpp(8,3): error C2065: 'bar': undeclared identifier\n");
    const QVector<TaskProblem> problems =
        matchTaskProblems(output, QStringLiteral("$msvc"), QStringLiteral("C:/proj"), {});
    QCOMPARE(problems.size(), 1);
    QCOMPARE(problems.front().line, 8);
    QCOMPARE(problems.front().column, 3);
    QCOMPARE(problems.front().severity, TaskProblem::Severity::Error);
}

void ProblemMatcherTest::ignoresUnknownMatcher()
{
    QVERIFY(matchTaskProblems(QStringLiteral("a.cpp:1:1: error: x"), QStringLiteral("none"), {},
                              {})
                .isEmpty());
}

QTEST_GUILESS_MAIN(ProblemMatcherTest)
#include "ProblemMatcherTest.moc"
