#ifndef EDITERAKO_BACKUPSERVICE_H
#define EDITERAKO_BACKUPSERVICE_H

#include "core/TextFileFormat.h"

#include <QList>
#include <QString>
#include <QtGlobal>

struct BackupBuffer {
    QString id;
    QString originalPath;
    QString displayName;
    QString lfText;
    TextFileMeta format;
    int caretPosition = 0;
    int caretAnchor = 0;
};

struct BackupSnapshot {
    QString workspace;
    QList<BackupBuffer> entries;
};

class BackupService
{
public:
    static constexpr qint64 kMaxContentBytes = 8LL * 1024 * 1024;

    [[nodiscard]] static QString defaultRoot();
    [[nodiscard]] static bool isSecretPath(const QString &path);
    [[nodiscard]] static bool exceedsSizeLimit(const QString &lfText);

    explicit BackupService(QString root = {});

    [[nodiscard]] QString root() const { return m_root; }
    [[nodiscard]] bool hasIndex() const;
    [[nodiscard]] bool writeSnapshot(const BackupSnapshot &snapshot) const;
    [[nodiscard]] BackupSnapshot loadSnapshot() const;
    void clear() const;

private:
    QString m_root;
};

#endif
