#include "core/DiskChangePolicy.h"

DiskChangeAction diskChangeAction(bool fileExists, bool editorIsModified)
{
    if (!fileExists) {
        return editorIsModified ? DiskChangeAction::WarnDeletedDirty
                                : DiskChangeAction::CloseTab;
    }
    return editorIsModified ? DiskChangeAction::PromptReload : DiskChangeAction::Reload;
}
