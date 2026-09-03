#ifndef EDITERAKO_WORKSPACEOPS_H
#define EDITERAKO_WORKSPACEOPS_H

#include <QString>

enum class WorkspaceDeleteMode {
    Trash,
    Permanent,
};

[[nodiscard]] QString uniqueSiblingPath(const QString &path);
[[nodiscard]] bool renameInsideWorkspace(const QString &workspaceRoot,
                                         const QString &path,
                                         const QString &newName,
                                         QString *resultPath = nullptr);
[[nodiscard]] bool deleteInsideWorkspace(const QString &workspaceRoot,
                                         const QString &path,
                                         WorkspaceDeleteMode mode);
[[nodiscard]] bool duplicateInsideWorkspace(const QString &workspaceRoot,
                                            const QString &path,
                                            QString *resultPath = nullptr);
[[nodiscard]] bool copyInsideWorkspace(const QString &workspaceRoot,
                                       const QString &source,
                                       const QString &destinationDirectory,
                                       QString *resultPath = nullptr);
[[nodiscard]] bool moveInsideWorkspace(const QString &workspaceRoot,
                                       const QString &source,
                                       const QString &destinationDirectory,
                                       QString *resultPath = nullptr);

#endif
