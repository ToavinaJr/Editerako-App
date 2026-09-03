#include "editor/EditorIo.h"

#include "core/AtomicFile.h"

#include <QTemporaryDir>
#include <QtTest>

class EditorIoTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyPathFails();
    void missingFileFails();
    void readsWrittenText();
    void preservesUnicode();
};

void EditorIoTest::emptyPathFails()
{
    const TextLoadResult result = readTextFile({});
    QVERIFY(!result.ok);
    QVERIFY(!result.error.isEmpty());
    QVERIFY(result.text.isEmpty());
}

void EditorIoTest::missingFileFails()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const TextLoadResult result = readTextFile(dir.filePath(QStringLiteral("absent.txt")));
    QVERIFY(!result.ok);
    QVERIFY(!result.error.isEmpty());
}

void EditorIoTest::readsWrittenText()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("note.txt"));

    QString error;
    QVERIFY(writeTextAtomically(path, QStringLiteral("hello\nworld"), &error));

    const TextLoadResult result = readTextFile(path);
    QVERIFY(result.ok);
    QCOMPARE(result.text, QStringLiteral("hello\nworld"));
}

void EditorIoTest::preservesUnicode()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("utf8.txt"));
    const QString text = QStringLiteral("café 你好 🚀");

    QString error;
    QVERIFY(writeTextAtomically(path, text, &error));

    const TextLoadResult result = readTextFile(path);
    QVERIFY(result.ok);
    QCOMPARE(result.text, text);
}

QTEST_GUILESS_MAIN(EditorIoTest)
#include "EditorIoTest.moc"
