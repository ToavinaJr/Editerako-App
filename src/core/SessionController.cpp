#include "core/SessionController.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

SessionController::RestoreGuard::RestoreGuard(SessionController &controller)
    : m_controller(controller)
{
    m_controller.beginRestore();
}

SessionController::RestoreGuard::~RestoreGuard()
{
    m_controller.endRestore();
}

void SessionController::beginRestore()
{
    m_restoring = true;
}

void SessionController::endRestore()
{
    m_restoring = false;
}

void SessionController::save(const SessionState &state)
{
    if (m_restoring) {
        return;
    }
    m_store.save(state);
}

SessionState SessionController::load() const
{
    return m_store.load();
}

void SessionController::save(const SessionState &state, QSettings &settings)
{
    if (m_restoring) {
        return;
    }
    m_store.save(state, settings);
}

SessionState SessionController::load(QSettings &settings) const
{
    return m_store.load(settings);
}

bool SessionController::workspaceIsRestorable(const SessionState &state)
{
    return !state.workspace.isEmpty() && QDir(state.workspace).exists();
}

QStringList SessionController::existingFiles(const QStringList &paths)
{
    QStringList existing;
    existing.reserve(paths.size());
    for (const QString &path : paths) {
        if (QFileInfo::exists(path)) {
            existing.append(path);
        }
    }
    return existing;
}
