#include "editor/DiagnosticMarkup.h"

#include <QPlainTextEdit>
#include <QtTest>

class DiagnosticMarkupTest : public QObject
{
    Q_OBJECT

private slots:
    void mapsRangeToSelection();
    void positionClampsToLine();
    void gutterWidthDependsOnDiagnostics();
};

void DiagnosticMarkupTest::mapsRangeToSelection()
{
    QPlainTextEdit editor;
    editor.setPlainText(QStringLiteral("int x;\nfloat y;\n"));
    EditorDiagnostic diag;
    diag.startLine = 1;
    diag.startCharacter = 0;
    diag.endLine = 1;
    diag.endCharacter = 5;
    diag.severity = EditorDiagnostic::Severity::Warning;
    diag.message = QStringLiteral("unused");
    const QList<QTextEdit::ExtraSelection> extras = diagnosticExtraSelections(&editor, {diag});
    QCOMPARE(extras.size(), 1);
    QCOMPARE(extras.front().cursor.selectedText(), QStringLiteral("float"));
}

void DiagnosticMarkupTest::positionClampsToLine()
{
    QPlainTextEdit editor;
    editor.setPlainText(QStringLiteral("ab"));
    QCOMPARE(documentPositionAt(editor.document(), 0, 99), 2);
    QCOMPARE(documentPositionAt(editor.document(), 40, 0), 2);
}

void DiagnosticMarkupTest::gutterWidthDependsOnDiagnostics()
{
    QCOMPARE(diagnosticGutterExtraWidth({}), 0);
    EditorDiagnostic diag;
    QVERIFY(diagnosticGutterExtraWidth({diag}) > 0);
}

QTEST_MAIN(DiagnosticMarkupTest)
#include "DiagnosticMarkupTest.moc"
