#include "core/KeybindingManager.h"

#include "core/CommandRegistry.h"

#include <QAction>
#include <QList>
#include <QSettings>

namespace {

QList<QKeySequence> shortcutsFor(const QString &commandId, const QKeySequence &primary)
{
    QList<QKeySequence> sequences;
    if (!primary.isEmpty()) {
        sequences.append(primary);
    }
    if (commandId == QLatin1String("editor.zoomIn")) {
        const QKeySequence equals(QStringLiteral("Ctrl+="));
        if (!sequences.contains(equals)) {
            sequences.append(equals);
        }
    }
    return sequences;
}

} // namespace

KeybindingManager::KeybindingManager(CommandRegistry *registry, QObject *parent)
    : QObject(parent)
    , m_registry(registry)
{
}

KeybindingManager::KeybindingManager(CommandRegistry *registry, QSettings &settings, QObject *parent)
    : QObject(parent)
    , m_registry(registry)
    , m_model(settings)
{
}

void KeybindingManager::applyOne(const QString &commandId)
{
    if (!m_registry) {
        return;
    }
    QAction *action = m_registry->action(commandId);
    if (!action) {
        return;
    }
    const QKeySequence primary = m_model.shortcut(commandId);
    const QList<QKeySequence> sequences = shortcutsFor(commandId, primary);
    if (sequences.size() > 1) {
        action->setShortcuts(sequences);
    } else {
        action->setShortcut(primary);
    }
}

void KeybindingManager::apply()
{
    if (!m_registry) {
        return;
    }
    for (const QString &id : m_registry->ids()) {
        applyOne(id);
    }
    emit shortcutsChanged();
}

bool KeybindingManager::setShortcut(const QString &commandId, const QKeySequence &sequence,
                                    QString *conflictId)
{
    if (!m_model.setShortcut(commandId, sequence, conflictId)) {
        return false;
    }
    applyOne(commandId);
    emit shortcutsChanged();
    return true;
}

void KeybindingManager::resetShortcut(const QString &commandId)
{
    m_model.resetToDefault(commandId);
    applyOne(commandId);
    emit shortcutsChanged();
}

QKeySequence KeybindingManager::shortcut(const QString &commandId) const
{
    return m_model.shortcut(commandId);
}
