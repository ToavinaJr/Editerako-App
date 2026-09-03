#ifndef EDITERAKO_GITPARSERS_H
#define EDITERAKO_GITPARSERS_H

#include "scm/SourceControlTypes.h"

#include <QByteArray>

namespace GitParsers {
[[nodiscard]] ScmStatus parseStatus(const QByteArray &output);
[[nodiscard]] QString parseRepositoryRoot(const QByteArray &output);
}

#endif

