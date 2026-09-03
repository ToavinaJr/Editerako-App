#ifndef EDITERAKO_DISKCHANGEPOLICY_H
#define EDITERAKO_DISKCHANGEPOLICY_H

enum class DiskChangeAction {
    WarnDeletedDirty,
    CloseTab,
    PromptReload,
    Reload,
};

// fileExists: QFileInfo::exists(path). editorIsModified: buffer dirty vs disk.
[[nodiscard]] DiskChangeAction diskChangeAction(bool fileExists, bool editorIsModified);

#endif
