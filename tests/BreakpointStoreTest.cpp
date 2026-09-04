#include "debug/BreakpointStore.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class BreakpointStoreTest : public QObject
{
    Q_OBJECT

private slots:
    void toggleAndQuery();
    void independentFiles();
};

void BreakpointStoreTest::toggleAndQuery()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("main.cpp"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("int main(){}\n");
    file.close();

    BreakpointStore store;
    QVERIFY(store.toggle(path, 3));
    QVERIFY(store.has(path, 3));
    QCOMPARE(store.sortedLinesFor(path), QList<int>{3});
    QVERIFY(!store.toggle(path, 3));
    QVERIFY(!store.has(path, 3));
    QVERIFY(store.isEmpty());
}

void BreakpointStoreTest::independentFiles()
{
    BreakpointStore store;
    QVERIFY(store.toggle(QStringLiteral("C:/a.cpp"), 1));
    QVERIFY(store.toggle(QStringLiteral("C:/b.cpp"), 2));
    QCOMPARE(store.sortedLinesFor(QStringLiteral("C:/a.cpp")), QList<int>{1});
    QCOMPARE(store.sortedLinesFor(QStringLiteral("C:/b.cpp")), QList<int>{2});
}

QTEST_GUILESS_MAIN(BreakpointStoreTest)
#include "BreakpointStoreTest.moc"
