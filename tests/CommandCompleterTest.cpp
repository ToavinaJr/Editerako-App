#include "terminal/CommandCompleter.h"
#include "terminal/CommandDiscovery.h"
#include "terminal/CommandHistory.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class CommandCompleterTest : public QObject
{
    Q_OBJECT

private slots:
    void gitCommandPrefix();
    void historyCommandIncluded();
    void gitArgumentPrefix();
    void pathSuggestionsMatchCwd();
    void suggestLimitsAndEmpty();
};

void CommandCompleterTest::gitCommandPrefix()
{
    CommandDiscovery discovery;
    discovery.seedBuiltins();
    CommandHistory history;
    CommandCompleter completer(&discovery, &history);

    const QStringList suggestions = completer.commandSuggestions(QStringLiteral("gi"));
    QVERIFY(suggestions.contains(QStringLiteral("git")));
}

void CommandCompleterTest::historyCommandIncluded()
{
    CommandDiscovery discovery;
    discovery.seedBuiltins();
    CommandHistory history;
    history.add(QStringLiteral("editerako-custom --help"));
    CommandCompleter completer(&discovery, &history);

    const QStringList suggestions = completer.commandSuggestions(QStringLiteral("editerako"));
    QVERIFY(suggestions.contains(QStringLiteral("editerako-custom")));
}

void CommandCompleterTest::gitArgumentPrefix()
{
    CommandDiscovery discovery;
    discovery.seedBuiltins();
    CommandHistory history;
    CommandCompleter completer(&discovery, &history);

    const QStringList suggestions = completer.argumentSuggestions(QStringLiteral("git"), QStringLiteral("sta"));
    QVERIFY(suggestions.contains(QStringLiteral("status")));
}

void CommandCompleterTest::pathSuggestionsMatchCwd()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QFile file(dir.filePath(QStringLiteral("alpha.txt")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    QVERIFY(QDir(dir.path()).mkdir(QStringLiteral("assets")));

    const QStringList files = CommandCompleter::pathSuggestions(QStringLiteral("al"), dir.path());
    QVERIFY(files.contains(QStringLiteral("alpha.txt")));

    const QStringList folders = CommandCompleter::pathSuggestions(QStringLiteral("as"), dir.path());
    QVERIFY(folders.contains(QStringLiteral("assets/")));
}

void CommandCompleterTest::suggestLimitsAndEmpty()
{
    CommandDiscovery discovery;
    discovery.seedBuiltins();
    CommandHistory history;
    CommandCompleter completer(&discovery, &history);

    QVERIFY(completer.suggest(QString(), QDir::tempPath()).isEmpty());
    const QStringList git = completer.suggest(QStringLiteral("git sta"), QDir::tempPath());
    QVERIFY(git.contains(QStringLiteral("status")));
}

QTEST_GUILESS_MAIN(CommandCompleterTest)
#include "CommandCompleterTest.moc"
