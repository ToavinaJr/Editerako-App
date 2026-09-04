#include "core/CommandRegistry.h"

#include <QAction>
#include <QWidget>
#include <QtTest>

class CommandRegistryTest : public QObject
{
    Q_OBJECT

private slots:
    void createAndLookup();
    void rejectsInvalidAndDuplicates();
    void setEnabled();
    void removeCommand();
};

void CommandRegistryTest::createAndLookup()
{
    QWidget parent;
    CommandRegistry registry(&parent);
    QAction *created = registry.create(QStringLiteral("file.save"), QStringLiteral("Save"));
    QVERIFY(created != nullptr);
    QCOMPARE(registry.action(QStringLiteral("file.save")), created);
    QCOMPARE(created->objectName(), QStringLiteral("file.save"));
}

void CommandRegistryTest::rejectsInvalidAndDuplicates()
{
    QWidget parent;
    CommandRegistry registry(&parent);
    QVERIFY(registry.add({}, new QAction(&parent)) == nullptr);
    QVERIFY(registry.add(QStringLiteral("x"), nullptr) == nullptr);

    QAction *first = registry.create(QStringLiteral("edit.find"), QStringLiteral("Find"));
    QVERIFY(first != nullptr);
    auto *second = new QAction(QStringLiteral("Find 2"), &parent);
    QCOMPARE(registry.add(QStringLiteral("edit.find"), second), first);
}

void CommandRegistryTest::setEnabled()
{
    QWidget parent;
    CommandRegistry registry(&parent);
    QAction *action = registry.create(QStringLiteral("view.term"), QStringLiteral("Term"));
    QVERIFY(registry.setEnabled(QStringLiteral("view.term"), false));
    QVERIFY(!action->isEnabled());
    QVERIFY(!registry.setEnabled(QStringLiteral("missing"), true));
}

void CommandRegistryTest::removeCommand()
{
    QWidget parent;
    CommandRegistry registry(&parent);
    QVERIFY(registry.create(QStringLiteral("plugin.hello"), QStringLiteral("Hello")));
    QVERIFY(registry.remove(QStringLiteral("plugin.hello")));
    QVERIFY(registry.action(QStringLiteral("plugin.hello")) == nullptr);
    QVERIFY(!registry.remove(QStringLiteral("plugin.hello")));
}

QTEST_MAIN(CommandRegistryTest)
#include "CommandRegistryTest.moc"
