#include "core/BackupService.h"
#include "core/RecoveryService.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class BackupServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void secretPathDetection();
    void sizeLimit();
    void roundTripFileAndUntitled();
    void skipsSecretsAndClearsWhenEmpty();
    void removesOrphanContent();
    void corruptIndexYieldsEmptySnapshot();
    void recoveryServiceDiscard();
};

void BackupServiceTest::secretPathDetection()
{
    QVERIFY(BackupService::isSecretPath(QStringLiteral("C:/proj/.env")));
    QVERIFY(BackupService::isSecretPath(QStringLiteral("/tmp/.env.local")));
    QVERIFY(BackupService::isSecretPath(QStringLiteral("/tmp/production.env")));
    QVERIFY(BackupService::isSecretPath(QStringLiteral("/home/u/.ssh/id_rsa")));
    QVERIFY(BackupService::isSecretPath(QStringLiteral("/keys/server.pem")));
    QVERIFY(BackupService::isSecretPath(QStringLiteral("credentials.json")));
    QVERIFY(!BackupService::isSecretPath(QString()));
    QVERIFY(!BackupService::isSecretPath(QStringLiteral("C:/proj/main.cpp")));
    QVERIFY(!BackupService::isSecretPath(QStringLiteral("C:/proj/.env.example")));
    QVERIFY(!BackupService::isSecretPath(QStringLiteral("/home/u/.ssh/id_rsa.pub")));
}

void BackupServiceTest::sizeLimit()
{
    QVERIFY(!BackupService::exceedsSizeLimit(QStringLiteral("hello")));
    const QByteArray huge(BackupService::kMaxContentBytes + 1, 'x');
    QVERIFY(BackupService::exceedsSizeLimit(QString::fromLatin1(huge)));
}

void BackupServiceTest::roundTripFileAndUntitled()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    BackupService store(dir.path());

    BackupBuffer file;
    file.id = QStringLiteral("aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee");
    file.originalPath = QStringLiteral("C:/proj/main.cpp");
    file.displayName = QStringLiteral("main.cpp");
    file.lfText = QStringLiteral("int main() {\n}\n");
    file.format.encoding = TextEncoding::Utf8;
    file.format.lineEnding = LineEnding::CrLf;
    file.format.bom = true;
    file.caretPosition = 4;
    file.caretAnchor = 1;

    BackupBuffer untitled;
    untitled.id = QStringLiteral("11111111-2222-3333-4444-555555555555");
    untitled.displayName = QStringLiteral("untitled");
    untitled.lfText = QStringLiteral("scratch");
    untitled.format.lineEnding = LineEnding::Lf;
    untitled.caretPosition = 2;
    untitled.caretAnchor = 2;

    BackupSnapshot snapshot;
    snapshot.workspace = QStringLiteral("C:/proj");
    snapshot.entries = {file, untitled};
    QVERIFY(store.writeSnapshot(snapshot));
    QVERIFY(store.hasIndex());

    const BackupSnapshot loaded = store.loadSnapshot();
    QCOMPARE(loaded.workspace, snapshot.workspace);
    QCOMPARE(loaded.entries.size(), 2);
    QCOMPARE(loaded.entries[0].originalPath, file.originalPath);
    QCOMPARE(loaded.entries[0].lfText, file.lfText);
    QCOMPARE(loaded.entries[0].format.lineEnding, LineEnding::CrLf);
    QVERIFY(loaded.entries[0].format.bom);
    QCOMPARE(loaded.entries[0].caretPosition, 4);
    QCOMPARE(loaded.entries[0].caretAnchor, 1);
    QCOMPARE(loaded.entries[1].originalPath, QString());
    QCOMPARE(loaded.entries[1].lfText, untitled.lfText);
}

void BackupServiceTest::skipsSecretsAndClearsWhenEmpty()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    BackupService store(dir.path());

    BackupBuffer secret;
    secret.id = QStringLiteral("aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee");
    secret.originalPath = QStringLiteral("C:/proj/.env");
    secret.displayName = QStringLiteral(".env");
    secret.lfText = QStringLiteral("API_KEY=secret");

    BackupSnapshot snapshot;
    snapshot.entries = {secret};
    QVERIFY(store.writeSnapshot(snapshot));
    QVERIFY(!store.hasIndex());
    QVERIFY(store.loadSnapshot().entries.isEmpty());
}

void BackupServiceTest::removesOrphanContent()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    BackupService store(dir.path());

    BackupBuffer first;
    first.id = QStringLiteral("aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee");
    first.displayName = QStringLiteral("untitled");
    first.lfText = QStringLiteral("one");

    BackupSnapshot snapshot;
    snapshot.entries = {first};
    QVERIFY(store.writeSnapshot(snapshot));
    QVERIFY(QFile::exists(dir.filePath(first.id + QStringLiteral(".txt"))));

    BackupBuffer second;
    second.id = QStringLiteral("11111111-2222-3333-4444-555555555555");
    second.displayName = QStringLiteral("untitled");
    second.lfText = QStringLiteral("two");
    snapshot.entries = {second};
    QVERIFY(store.writeSnapshot(snapshot));

    QVERIFY(!QFile::exists(dir.filePath(first.id + QStringLiteral(".txt"))));
    QVERIFY(QFile::exists(dir.filePath(second.id + QStringLiteral(".txt"))));
}

void BackupServiceTest::corruptIndexYieldsEmptySnapshot()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QFile index(dir.filePath(QStringLiteral("index.json")));
    QVERIFY(index.open(QIODevice::WriteOnly));
    index.write("{not json");
    index.close();

    const BackupSnapshot loaded = BackupService(dir.path()).loadSnapshot();
    QVERIFY(loaded.entries.isEmpty());
}

void BackupServiceTest::recoveryServiceDiscard()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    RecoveryService recovery(dir.path());
    QVERIFY(!recovery.canRecover());

    BackupBuffer untitled;
    untitled.id = QStringLiteral("aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee");
    untitled.displayName = QStringLiteral("untitled");
    untitled.lfText = QStringLiteral("keep me");

    BackupSnapshot snapshot;
    snapshot.workspace = QStringLiteral("/ws");
    snapshot.entries = {untitled};
    QVERIFY(recovery.save(snapshot));
    QVERIFY(recovery.canRecover());
    QCOMPARE(recovery.load().entries.size(), 1);

    recovery.discard();
    QVERIFY(!recovery.canRecover());
}

QTEST_GUILESS_MAIN(BackupServiceTest)
#include "BackupServiceTest.moc"
