#include "editor/ProblemModel.h"

#include <QtTest>

class ProblemModelTest : public QObject
{
    Q_OBJECT

private slots:
    void replacesPerFileAndCounts();
    void filterErrorsAndWarnings();
    void clearFileRemovesOnlyThatPath();
    void emptyDiagnosticsClearFile();
    void setSourceProblemsKeepsOtherSources();
};

void ProblemModelTest::replacesPerFileAndCounts()
{
    ProblemModel model;
    ProblemItem err;
    err.path = QStringLiteral("/a.cpp");
    err.line = 2;
    err.severity = EditorDiagnostic::Severity::Error;
    err.message = QStringLiteral("bad");
    ProblemItem warn;
    warn.path = QStringLiteral("/a.cpp");
    warn.line = 1;
    warn.severity = EditorDiagnostic::Severity::Warning;
    warn.message = QStringLiteral("hmm");
    model.setFileProblems(QStringLiteral("/a.cpp"), {err, warn});
    QCOMPARE(model.errorCount(), 1);
    QCOMPARE(model.warningCount(), 1);
    QCOMPARE(model.totalCount(), 2);
    QCOMPARE(model.visibleItems().front().line, 1);
}

void ProblemModelTest::filterErrorsAndWarnings()
{
    ProblemModel model;
    ProblemItem err;
    err.path = QStringLiteral("/a.cpp");
    err.severity = EditorDiagnostic::Severity::Error;
    ProblemItem warn;
    warn.path = QStringLiteral("/a.cpp");
    warn.severity = EditorDiagnostic::Severity::Warning;
    ProblemItem info;
    info.path = QStringLiteral("/a.cpp");
    info.severity = EditorDiagnostic::Severity::Information;
    model.setFileProblems(QStringLiteral("/a.cpp"), {err, warn, info});

    model.setFilter(ProblemModel::Filter::Errors);
    QCOMPARE(model.visibleItems().size(), 1);
    QCOMPARE(model.visibleItems().front().severity, EditorDiagnostic::Severity::Error);

    model.setFilter(ProblemModel::Filter::Warnings);
    QCOMPARE(model.visibleItems().size(), 1);
    QCOMPARE(model.visibleItems().front().severity, EditorDiagnostic::Severity::Warning);

    model.setFilter(ProblemModel::Filter::All);
    QCOMPARE(model.visibleItems().size(), 3);
}

void ProblemModelTest::clearFileRemovesOnlyThatPath()
{
    ProblemModel model;
    ProblemItem a;
    a.path = QStringLiteral("/a.cpp");
    a.severity = EditorDiagnostic::Severity::Error;
    ProblemItem b;
    b.path = QStringLiteral("/b.cpp");
    b.severity = EditorDiagnostic::Severity::Warning;
    model.setFileProblems(QStringLiteral("/a.cpp"), {a});
    model.setFileProblems(QStringLiteral("/b.cpp"), {b});
    model.clearFile(QStringLiteral("/a.cpp"));
    QCOMPARE(model.errorCount(), 0);
    QCOMPARE(model.warningCount(), 1);
    QCOMPARE(model.visibleItems().front().path, QStringLiteral("/b.cpp"));
}

void ProblemModelTest::emptyDiagnosticsClearFile()
{
    ProblemModel model;
    ProblemItem a;
    a.path = QStringLiteral("/a.cpp");
    a.severity = EditorDiagnostic::Severity::Error;
    model.setFileProblems(QStringLiteral("/a.cpp"), {a});
    model.setFileProblems(QStringLiteral("/a.cpp"), {});
    QVERIFY(model.isEmpty());
    QCOMPARE(model.totalCount(), 0);
}

void ProblemModelTest::setSourceProblemsKeepsOtherSources()
{
    ProblemModel model;
    ProblemItem lsp;
    lsp.path = QStringLiteral("/a.cpp");
    lsp.severity = EditorDiagnostic::Severity::Error;
    lsp.message = QStringLiteral("lsp");
    lsp.source = QStringLiteral("clangd");
    model.setFileProblems(QStringLiteral("/a.cpp"), {lsp});

    ProblemItem task;
    task.path = QStringLiteral("/a.cpp");
    task.line = 4;
    task.severity = EditorDiagnostic::Severity::Warning;
    task.message = QStringLiteral("gcc");
    model.setSourceProblems(QStringLiteral("task"), {task});
    QCOMPARE(model.totalCount(), 2);

    model.setSourceProblems(QStringLiteral("task"), {});
    QCOMPARE(model.totalCount(), 1);
    QCOMPARE(model.visibleItems().front().source, QStringLiteral("clangd"));
}

QTEST_GUILESS_MAIN(ProblemModelTest)
#include "ProblemModelTest.moc"
