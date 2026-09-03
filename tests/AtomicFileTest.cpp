#include "core/AtomicFile.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QtTest>

class AtomicFileTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyPathFails();
    void writesAndOverwrites();
    void preservesUnicode();
    void directoryPathFails();
    void writeBytesKeepsRawEol();
};

void AtomicFileTest::emptyPathFails()
{
    QString error;
    QVERIFY(!writeTextAtomically({}, QStringLiteral("x"), &error));
    QVERIFY(!error.isEmpty());
}

void AtomicFileTest::writesAndOverwrites()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("note.txt"));

    QString error;
    QVERIFY(writeTextAtomically(path, QStringLiteral("hello"), &error));
    QCOMPARE(error, QString());

    QFile first(path);
    QVERIFY(first.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QTextStream(&first).readAll(), QStringLiteral("hello"));
    first.close();

    QVERIFY(writeTextAtomically(path, QStringLiteral("world\nline2"), &error));
    QFile second(path);
    QVERIFY(second.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QTextStream(&second).readAll(), QStringLiteral("world\nline2"));
}

void AtomicFileTest::preservesUnicode()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("utf8.txt"));
    const QString text = QStringLiteral("café 你好 🚀");

    QString error;
    QVERIFY(writeTextAtomically(path, text, &error));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QTextStream(&file).readAll(), text);
}

void AtomicFileTest::directoryPathFails()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString error;
    QVERIFY(!writeTextAtomically(dir.path(), QStringLiteral("x"), &error));
    QVERIFY(!error.isEmpty());
}

void AtomicFileTest::writeBytesKeepsRawEol()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("raw.txt"));
    const QByteArray payload("a\r\nb");
    QString error;
    QVERIFY(writeBytesAtomically(path, payload, &error));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), payload);
}

QTEST_GUILESS_MAIN(AtomicFileTest)
#include "AtomicFileTest.moc"
