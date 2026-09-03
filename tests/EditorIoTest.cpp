#include "editor/EditorIo.h"

#include "core/AtomicFile.h"

#include <QFile>
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
    void preservesCrLfOnDisk();
    void diskMatchesEncodedBytes();
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
    QCOMPARE(result.meta.encoding, TextEncoding::Utf8);
    QCOMPARE(result.meta.lineEnding, LineEnding::Lf);
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

void EditorIoTest::preservesCrLfOnDisk()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("win.txt"));

    TextFileMeta meta;
    meta.encoding = TextEncoding::Utf8;
    meta.lineEnding = LineEnding::CrLf;
    const TextSaveResult saved = writeTextFile(path, QStringLiteral("a\nb"), meta);
    QVERIFY(saved.ok);

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("a\r\nb"));
    file.close();

    const TextLoadResult loaded = readTextFile(path);
    QVERIFY(loaded.ok);
    QCOMPARE(loaded.text, QStringLiteral("a\nb"));
    QCOMPARE(loaded.meta.lineEnding, LineEnding::CrLf);
}

void EditorIoTest::diskMatchesEncodedBytes()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("match.txt"));

    TextFileMeta meta;
    meta.encoding = TextEncoding::Utf8;
    meta.lineEnding = LineEnding::Lf;
    QVERIFY(writeTextFile(path, QStringLiteral("x\ny"), meta).ok);
    QVERIFY(diskMatches(path, QStringLiteral("x\ny"), meta));
    QVERIFY(!diskMatches(path, QStringLiteral("changed"), meta));
}

QTEST_GUILESS_MAIN(EditorIoTest)
#include "EditorIoTest.moc"
