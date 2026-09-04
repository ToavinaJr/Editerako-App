#ifndef EDITERAKO_COMMANDREGISTRY_H
#define EDITERAKO_COMMANDREGISTRY_H

#include <QHash>
#include <QKeySequence>
#include <QObject>
#include <QString>
#include <QStringList>

class QAction;
class QWidget;

class CommandRegistry : public QObject
{
    Q_OBJECT

public:
    explicit CommandRegistry(QWidget *actionParent);

    QAction *add(const QString &id, QAction *action);
    QAction *create(const QString &id,
                    const QString &text,
                    const QKeySequence &shortcut = {});

    [[nodiscard]] QAction *action(const QString &id) const;
    [[nodiscard]] QStringList ids() const;
    bool setEnabled(const QString &id, bool enabled);
    bool remove(const QString &id);

private:
    QWidget *m_actionParent = nullptr;
    QHash<QString, QAction *> m_actions;
};

#endif
