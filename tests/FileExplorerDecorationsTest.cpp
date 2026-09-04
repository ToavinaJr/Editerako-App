#include "project/FileExplorerDecorations.h"

#include <QtTest>

class FileExplorerDecorationsTest : public QObject
{
    Q_OBJECT
private slots:
    void fileIconsByExtension();
    void badgeColors();
    void itemTextWithAndWithoutBadge();
};

void FileExplorerDecorationsTest::fileIconsByExtension()
{
    QCOMPARE(fileExplorerFileIcon(QStringLiteral("main.cpp")), QStringLiteral("🔵"));
    QCOMPARE(fileExplorerFileIcon(QStringLiteral("foo.H")), QStringLiteral("🟦"));
    QCOMPARE(fileExplorerFileIcon(QStringLiteral("readme.md")), QStringLiteral("📄"));
}

void FileExplorerDecorationsTest::badgeColors()
{
    QCOMPARE(fileExplorerBadgeColor(QStringLiteral("M")), QColor(QStringLiteral("#d29922")));
    QCOMPARE(fileExplorerBadgeColor(QStringLiteral("A")), QColor(QStringLiteral("#3fb950")));
    QCOMPARE(fileExplorerBadgeColor(QStringLiteral("D")), QColor(QStringLiteral("#f85149")));
    QCOMPARE(fileExplorerBadgeColor(QStringLiteral("?")), QColor(QStringLiteral("#8b949e")));
}

void FileExplorerDecorationsTest::itemTextWithAndWithoutBadge()
{
    QCOMPARE(fileExplorerItemText(QStringLiteral("📄"), QStringLiteral("a.txt"), {}),
             QStringLiteral("📄 a.txt"));
    QCOMPARE(fileExplorerItemText(QStringLiteral("🔵"), QStringLiteral("a.cpp"), QStringLiteral("M")),
             QStringLiteral("🔵 a.cpp  M"));
}

QTEST_GUILESS_MAIN(FileExplorerDecorationsTest)
#include "FileExplorerDecorationsTest.moc"
