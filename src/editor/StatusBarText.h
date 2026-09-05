#ifndef EDITERAKO_STATUSBARTEXT_H
#define EDITERAKO_STATUSBARTEXT_H

#include <QString>

[[nodiscard]] QString statusBarPositionLabel(int line, int column);
[[nodiscard]] QString statusBarIndentModeLabel(bool insertSpaces);
[[nodiscard]] QString statusBarTabSizeLabel(int tabSize);
[[nodiscard]] QString statusBarProblemsLabel(int errors, int warnings);

#endif
