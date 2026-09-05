#ifndef EDITERAKO_SOURCECONTROLTYPES_H
#define EDITERAKO_SOURCECONTROLTYPES_H

#include <QMetaType>
#include <QString>
#include <QVector>

enum class ScmFileState {
    Modified,
    Added,
    Deleted,
    Renamed,
    Copied,
    Untracked,
    Conflicted,
    Unknown
};

struct ScmChange {
    QString path;
    QString oldPath;
    ScmFileState state = ScmFileState::Unknown;
    bool staged = false;
};

struct ScmStatus {
    bool isRepository = false;
    QString repositoryRoot;
    QString branch;
    int ahead = 0;
    int behind = 0;
    QVector<ScmChange> changes;
};

struct ScmError {
    QString message;
    int exitCode = 0;
};

Q_DECLARE_METATYPE(ScmChange)
Q_DECLARE_METATYPE(ScmStatus)
Q_DECLARE_METATYPE(ScmError)

#endif

