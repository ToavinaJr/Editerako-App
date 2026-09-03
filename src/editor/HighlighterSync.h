#ifndef EDITERAKO_HIGHLIGHTERSYNC_H
#define EDITERAKO_HIGHLIGHTERSYNC_H

#include "syntax/LanguageRegistry.h"

#include <QtGlobal>

class CodeEditor;

namespace HighlighterSync {

[[nodiscard]] bool shouldHighlight(LanguageId language, qint64 sizeBytes, qint64 disableThreshold);

void apply(CodeEditor *editor);

}

#endif
