#include "lsp/LspCompileCommands.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QTemporaryDir>
#include <QtTest>

class LspCompileCommandsTest : public QObject
{
    Q_OBJECT
private slots:
    void emptyRoot();
    void prefersRootFile();
    void findsBuildPreset();
    void prefersNewerPreset();
    void acceptsFilePath();
};

namespace {

bool writeCompileCommands(const QString &dir)
{
    QDir().mkpath(dir);
    QFile file(QDir(dir).filePath(QStringLiteral("compile_commands.json")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write("[]\n");
    return true;
}

} // namespace

void LspCompileCommandsTest::emptyRoot()
{
    QCOMPARE(lspCompileCommandsDir({}), QString());
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QCOMPARE(lspCompileCommandsDir(tmp.path()), QString());
}

void LspCompileCommandsTest::prefersRootFile()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(writeCompileCommands(tmp.path()));
    QCOMPARE(QDir(lspCompileCommandsDir(tmp.path())).canonicalPath(),
             QDir(tmp.path()).canonicalPath());
}

void LspCompileCommandsTest::findsBuildPreset()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString debugDir = QDir(tmp.path()).filePath(QStringLiteral("build/debug"));
    QVERIFY(writeCompileCommands(debugDir));
    QCOMPARE(QDir(lspCompileCommandsDir(tmp.path())).canonicalPath(),
             QDir(debugDir).canonicalPath());
}

void LspCompileCommandsTest::prefersNewerPreset()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString debugDir = QDir(tmp.path()).filePath(QStringLiteral("build/debug"));
    const QString releaseDir = QDir(tmp.path()).filePath(QStringLiteral("build/release"));
    QVERIFY(writeCompileCommands(debugDir));
    QVERIFY(writeCompileCommands(releaseDir));

    QFile debugFile(QDir(debugDir).filePath(QStringLiteral("compile_commands.json")));
    QFile releaseFile(QDir(releaseDir).filePath(QStringLiteral("compile_commands.json")));
    QVERIFY(debugFile.open(QIODevice::ReadWrite));
    QVERIFY(releaseFile.open(QIODevice::ReadWrite));
    const QDateTime older = QDateTime::currentDateTimeUtc().addSecs(-120);
    const QDateTime newer = QDateTime::currentDateTimeUtc();
    QVERIFY(debugFile.setFileTime(older, QFileDevice::FileModificationTime));
    QVERIFY(releaseFile.setFileTime(newer, QFileDevice::FileModificationTime));

    QCOMPARE(QDir(lspCompileCommandsDir(tmp.path())).canonicalPath(),
             QDir(releaseDir).canonicalPath());
}

void LspCompileCommandsTest::acceptsFilePath()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString debugDir = QDir(tmp.path()).filePath(QStringLiteral("build/debug"));
    QVERIFY(writeCompileCommands(debugDir));
    const QString cpp = QDir(tmp.path()).filePath(QStringLiteral("src/app/foo.cpp"));
    QDir().mkpath(QFileInfo(cpp).absolutePath());
    QFile file(cpp);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(QDir(lspCompileCommandsDir(cpp)).canonicalPath(), QDir(debugDir).canonicalPath());
}

QTEST_GUILESS_MAIN(LspCompileCommandsTest)
#include "LspCompileCommandsTest.moc"
