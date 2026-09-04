#ifndef EDITERAKO_GITPARSERS_H
#define EDITERAKO_GITPARSERS_H

#include "scm/SourceControlTypes.h"

#include <QByteArray>
#include <QHash>
#include <QString>

namespace GitParsers {
[[nodiscard]] ScmStatus parseStatus(const QByteArray &output);
[[nodiscard]] QString parseRepositoryRoot(const QByteArray &output);
void makePathsAbsolute(ScmStatus &status, const QString &repositoryRoot);
[[nodiscard]] QString badgeFor(ScmFileState state);
[[nodiscard]] QHash<QString, QString> explorerBadges(const ScmStatus &status);
[[nodiscard]] QString branchName(const ScmStatus &status);
}

#endif

