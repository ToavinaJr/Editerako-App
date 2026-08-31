#include "core/CommandRegistry.h"

#include "core/Logging.h"

#include <QAction>
#include <QWidget>

CommandRegistry::CommandRegistry(QWidget *actionParent)
    : QObject(actionParent)
    , m_actionParent(actionParent)
{
}

QAction *CommandRegistry::add(const QString &id, QAction *action)
{
    if (id.isEmpty() || !action) {
        qCWarning(lcCore) << "Refusing to register invalid command" << id;
        return nullptr;
    }
    if (m_actions.contains(id)) {
        qCWarning(lcCore) << "Command already registered" << id;
        return m_actions.value(id);
    }

    action->setObjectName(id);
    m_actions.insert(id, action);
    return action;
}

QAction *CommandRegistry::create(const QString &id,
                                 const QString &text,
                                 const QKeySequence &shortcut)
{
    if (!m_actionParent) {
        qCWarning(lcCore) << "Cannot create command without an action parent" << id;
        return nullptr;
    }

    auto *action = new QAction(text, m_actionParent);
    action->setShortcut(shortcut);
    action->setShortcutContext(Qt::WindowShortcut);
    m_actionParent->addAction(action);
    return add(id, action);
}

QAction *CommandRegistry::action(const QString &id) const
{
    return m_actions.value(id, nullptr);
}

bool CommandRegistry::setEnabled(const QString &id, bool enabled)
{
    QAction *cmd = action(id);
    if (!cmd) {
        return false;
    }
    cmd->setEnabled(enabled);
    return true;
}
