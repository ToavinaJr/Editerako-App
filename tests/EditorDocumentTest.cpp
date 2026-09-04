#include "editor/EditorDocument.h"

#include "editor/CodeEditor.h"

#include <QTemporaryDir>
#include <QTextCursor>
#include <QtTest>

class EditorDocumentTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultFormatIsPlatform();
    void languageFollowsPath();
    void versionIncrementsAndResets();
    void caretStateRoundtrip();
    void backupIdPersistsOnceGenerated();
};

void EditorDocumentTest::defaultFormatIsPlatform()
{
    CodeEditor editor;
    EditorDocument doc(&editor);
    QCOMPARE(doc.format().encoding, TextEncoding::Utf8);
    QCOMPARE(doc.format().lineEnding, platformLineEnding());
    QVERIFY(!doc.format().bom);
    QVERIFY(!doc.isReadOnly());
    QCOMPARE(doc.language(), LanguageId::PlainText);
}

void EditorDocumentTest::languageFollowsPath()
{
    CodeEditor editor;
    EditorDocument doc(&editor);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    doc.setFilePath(dir.filePath(QStringLiteral("main.cpp")));
    QCOMPARE(doc.language(), LanguageId::Cpp);
    QCOMPARE(doc.displayName(), QStringLiteral("main.cpp"));
}

void EditorDocumentTest::versionIncrementsAndResets()
{
    CodeEditor editor;
    EditorDocument doc(&editor);
    const int initial = doc.version();
    editor.setPlainText(QStringLiteral("hello"));
    QVERIFY(doc.version() > initial);
    doc.resetVersion();
    QCOMPARE(doc.version(), 0);
}

void EditorDocumentTest::caretStateRoundtrip()
{
    CodeEditor editor;
    EditorDocument doc(&editor);
    editor.setPlainText(QStringLiteral("abc"));
    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(2);
    editor.setTextCursor(cursor);

    const EditorDocument::CaretState state = doc.caretState();
    QCOMPARE(state.position, 2);

    EditorDocument::CaretState restored = state;
    restored.position = 1;
    restored.anchor = 1;
    doc.restoreCaretState(restored);
    QCOMPARE(editor.textCursor().position(), 1);
}

void EditorDocumentTest::backupIdPersistsOnceGenerated()
{
    CodeEditor editor;
    EditorDocument doc(&editor);
    QVERIFY(doc.backupId().isEmpty());
    const QString first = doc.ensureBackupId();
    QVERIFY(!first.isEmpty());
    QCOMPARE(doc.ensureBackupId(), first);
    doc.setBackupId(QStringLiteral("aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"));
    QCOMPARE(doc.backupId(), QStringLiteral("aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"));
}

QTEST_MAIN(EditorDocumentTest)
#include "EditorDocumentTest.moc"
