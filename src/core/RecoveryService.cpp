#include "core/RecoveryService.h"

RecoveryService::RecoveryService(QString root)
    : m_backup(std::move(root))
{
}

bool RecoveryService::canRecover() const
{
    return !m_backup.loadSnapshot().entries.isEmpty();
}

BackupSnapshot RecoveryService::load() const
{
    return m_backup.loadSnapshot();
}

bool RecoveryService::save(const BackupSnapshot &snapshot) const
{
    return m_backup.writeSnapshot(snapshot);
}

void RecoveryService::discard() const
{
    m_backup.clear();
}
