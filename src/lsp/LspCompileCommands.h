#ifndef EDITERAKO_LSPCOMPILECOMMANDS_H
#define EDITERAKO_LSPCOMPILECOMMANDS_H

#include <QString>

// Directory that contains compile_commands.json, or empty if none is found
// under workspaceRoot (root, build/, build/<preset>/).
[[nodiscard]] QString lspCompileCommandsDir(const QString &workspaceRoot);

#endif
