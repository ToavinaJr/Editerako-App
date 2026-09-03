#ifndef EDITERAKO_WORKSPACEPATH_H
#define EDITERAKO_WORKSPACEPATH_H

#include <QString>

[[nodiscard]] bool isSafeRelativePath(const QString &name);
[[nodiscard]] QString normalizePath(const QString &path);
[[nodiscard]] bool isInsideWorkspace(const QString &workspaceRoot, const QString &path);
[[nodiscard]] QString resolveInsideWorkspace(const QString &workspaceRoot,
                                             const QString &directory,
                                             const QString &relativeName);

#endif
