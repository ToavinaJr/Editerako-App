#ifndef EDITERAKO_RECOVERYSERVICE_H
#define EDITERAKO_RECOVERYSERVICE_H

#include "core/BackupService.h"

class RecoveryService
{
public:
    explicit RecoveryService(QString root = {});

    [[nodiscard]] QString root() const { return m_backup.root(); }
    [[nodiscard]] bool canRecover() const;
    [[nodiscard]] BackupSnapshot load() const;
    bool save(const BackupSnapshot &snapshot) const;
    void discard() const;

private:
    BackupService m_backup;
};

#endif
