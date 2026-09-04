#include "core/CommandRegistry.h"
#include "core/KeybindingManager.h"
#include "core/KeybindingModel.h"

#include <QAction>
#include <QList>
#include <QSettings>
#include <QTemporaryDir>
#include <QWidget>
#include <QtTest>

class KeybindingModelTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultsIncludeSaveAndTerminal();
    void overrideAndReset();
    void detectsConflicts();
    void appliesToRegistry();
    void zoomInIncludesEqualsAndPlus();
};

void KeybindingModelTest::defaultsIncludeSaveAndTerminal()
{
    const auto defaults = KeybindingModel::defaultShortcuts();
    QVERIFY(defaults.contains(QStringLiteral("file.save")));
    QCOMPARE(defaults.value(QStringLiteral("file.close")),
             QKeySequence(QStringLiteral("Ctrl+W")));
    QCOMPARE(defaults.value(QStringLiteral("file.markdownPreview")),
             QKeySequence(QStringLiteral("Ctrl+Shift+V")));
    QCOMPARE(defaults.value(QStringLiteral("view.terminal")),
             QKeySequence(QStringLiteral("Ctrl+J")));
    QCOMPARE(defaults.value(QStringLiteral("workbench.problems")),
             QKeySequence(QStringLiteral("Ctrl+Shift+M")));
    QCOMPARE(defaults.value(QStringLiteral("workbench.sourceControl")),
             QKeySequence(QStringLiteral("Ctrl+Shift+G")));
    QCOMPARE(defaults.value(QStringLiteral("workbench.build")),
             QKeySequence(QStringLiteral("Ctrl+Shift+B")));
    QCOMPARE(defaults.value(QStringLiteral("workbench.commandPalette")),
             QKeySequence(QStringLiteral("Ctrl+Shift+P")));
    QCOMPARE(defaults.value(QStringLiteral("workbench.quickOpen")),
             QKeySequence(QStringLiteral("Ctrl+P")));
    QCOMPARE(defaults.value(QStringLiteral("workbench.search")),
             QKeySequence(QStringLiteral("Ctrl+Shift+F")));
    QCOMPARE(defaults.value(QStringLiteral("edit.toggleLineComment")),
             QKeySequence(QStringLiteral("Ctrl+/")));
    QCOMPARE(defaults.value(QStringLiteral("edit.moveLineUp")),
             QKeySequence(QStringLiteral("Ctrl+Up")));
    QCOMPARE(defaults.value(QStringLiteral("editor.toggleFold")),
             QKeySequence(QStringLiteral("Ctrl+Shift+[")));
    QCOMPARE(defaults.value(QStringLiteral("editor.foldAll")),
             QKeySequence(QStringLiteral("Ctrl+Alt+[")));
    QCOMPARE(defaults.value(QStringLiteral("editor.unfoldAll")),
             QKeySequence(QStringLiteral("Ctrl+Alt+]")));
    QCOMPARE(defaults.value(QStringLiteral("editor.gotoDefinition")),
             QKeySequence(QStringLiteral("F12")));
    QCOMPARE(defaults.value(QStringLiteral("editor.findReferences")),
             QKeySequence(QStringLiteral("Shift+F12")));
    QCOMPARE(defaults.value(QStringLiteral("editor.triggerSuggest")),
             QKeySequence(QStringLiteral("Ctrl+Space")));
    QCOMPARE(defaults.value(QStringLiteral("editor.zoomIn")),
             QKeySequence(QStringLiteral("Ctrl++")));
    QCOMPARE(defaults.value(QStringLiteral("editor.zoomOut")),
             QKeySequence(QStringLiteral("Ctrl+-")));
    QCOMPARE(defaults.value(QStringLiteral("editor.zoomReset")),
             QKeySequence(QStringLiteral("Ctrl+0")));
    QCOMPARE(defaults.value(QStringLiteral("workbench.splitEditorRight")),
             QKeySequence(QStringLiteral("Ctrl+\\")));
    QCOMPARE(defaults.value(QStringLiteral("workbench.splitEditorDown")),
             QKeySequence(QStringLiteral("Ctrl+Shift+\\")));
    QCOMPARE(defaults.value(QStringLiteral("workbench.debug")),
             QKeySequence(QStringLiteral("Ctrl+Shift+Y")));
    QCOMPARE(defaults.value(QStringLiteral("debug.startContinue")),
             QKeySequence(QStringLiteral("F5")));
    QCOMPARE(defaults.value(QStringLiteral("debug.stop")),
             QKeySequence(QStringLiteral("Shift+F5")));
    QCOMPARE(defaults.value(QStringLiteral("debug.toggleBreakpoint")),
             QKeySequence(QStringLiteral("F9")));
    QCOMPARE(defaults.value(QStringLiteral("debug.stepOver")),
             QKeySequence(QStringLiteral("F10")));
    QCOMPARE(defaults.value(QStringLiteral("debug.stepInto")),
             QKeySequence(QStringLiteral("F11")));
    QCOMPARE(defaults.value(QStringLiteral("debug.stepOut")),
             QKeySequence(QStringLiteral("Shift+F11")));
}

void KeybindingModelTest::overrideAndReset()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings store(dir.filePath(QStringLiteral("keys.ini")), QSettings::IniFormat);
    KeybindingModel model(store);

    QVERIFY(model.setShortcut(QStringLiteral("file.close"), QKeySequence(QStringLiteral("Ctrl+Q"))));
    QCOMPARE(model.shortcut(QStringLiteral("file.close")), QKeySequence(QStringLiteral("Ctrl+Q")));

    model.resetToDefault(QStringLiteral("file.close"));
    QCOMPARE(model.shortcut(QStringLiteral("file.close")), QKeySequence(QStringLiteral("Ctrl+W")));
}

void KeybindingModelTest::detectsConflicts()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings store(dir.filePath(QStringLiteral("keys.ini")), QSettings::IniFormat);
    KeybindingModel model(store);

    QString conflict;
    QVERIFY(!model.setShortcut(QStringLiteral("file.new"),
                               QKeySequence(QStringLiteral("Ctrl+S")),
                               &conflict));
    QCOMPARE(conflict, QStringLiteral("file.save"));
}

void KeybindingModelTest::appliesToRegistry()
{
    QWidget parent;
    CommandRegistry registry(&parent);
    QAction *save = registry.create(QStringLiteral("file.save"), QStringLiteral("Save"));
    QVERIFY(save != nullptr);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings store(dir.filePath(QStringLiteral("keys.ini")), QSettings::IniFormat);
    KeybindingManager manager(&registry, store);
    manager.apply();
    QCOMPARE(save->shortcut(), QKeySequence::Save);

    QVERIFY(manager.setShortcut(QStringLiteral("file.save"), QKeySequence(QStringLiteral("Ctrl+E"))));
    QCOMPARE(save->shortcut(), QKeySequence(QStringLiteral("Ctrl+E")));
}

void KeybindingModelTest::zoomInIncludesEqualsAndPlus()
{
    QWidget parent;
    CommandRegistry registry(&parent);
    QAction *zoomIn = registry.create(QStringLiteral("editor.zoomIn"), QStringLiteral("Zoom In"));
    QVERIFY(zoomIn != nullptr);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings store(dir.filePath(QStringLiteral("keys.ini")), QSettings::IniFormat);
    KeybindingManager manager(&registry, store);
    manager.apply();

    const QList<QKeySequence> sequences = zoomIn->shortcuts();
    QVERIFY(sequences.contains(QKeySequence(QStringLiteral("Ctrl++"))));
    QVERIFY(sequences.contains(QKeySequence(QStringLiteral("Ctrl+="))));
}

QTEST_MAIN(KeybindingModelTest)
#include "KeybindingModelTest.moc"
