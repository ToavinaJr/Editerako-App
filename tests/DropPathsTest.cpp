#include "core/DropPaths.h"

#include <QDir>
#include <QMimeData>
#include <QUrl>
#include <QtTest>

class DropPathsTest : public QObject
{
    Q_OBJECT

private slots:
    void nullMimeIsEmpty();
    void noUrlsIsEmpty();
    void localFileUrls();
};

void DropPathsTest::nullMimeIsEmpty()
{
    QVERIFY(localPathsFromMimeData(nullptr).isEmpty());
}

void DropPathsTest::noUrlsIsEmpty()
{
    QMimeData mime;
    mime.setText(QStringLiteral("hello"));
    QVERIFY(localPathsFromMimeData(&mime).isEmpty());
}

void DropPathsTest::localFileUrls()
{
    const QString path = QDir::fromNativeSeparators(QDir::temp().absoluteFilePath(QStringLiteral("editerako-drop.txt")));
    QMimeData mime;
    mime.setUrls({QUrl::fromLocalFile(path)});
    const QStringList paths = localPathsFromMimeData(&mime);
    QCOMPARE(paths.size(), 1);
    QCOMPARE(QDir::fromNativeSeparators(paths.first()), path);
}

QTEST_GUILESS_MAIN(DropPathsTest)
#include "DropPathsTest.moc"
