#include "editor/StatusBarText.h"

#include <QCoreApplication>
#include <QtGlobal>

namespace {

QString trStatus(const char *source)
{
    return QCoreApplication::translate("EditorStatusWidget", source);
}

} // namespace

QString statusBarPositionLabel(int line, int column)
{
    return trStatus("Ln %1, Col %2").arg(qMax(1, line)).arg(qMax(1, column));
}

QString statusBarIndentModeLabel(bool insertSpaces)
{
    return insertSpaces ? trStatus("Spaces") : trStatus("Tabs");
}

QString statusBarTabSizeLabel(int tabSize)
{
    return trStatus("Tab Size: %1").arg(qBound(1, tabSize, 16));
}

QString statusBarProblemsLabel(int errors, int warnings)
{
    const int safeErrors = qMax(0, errors);
    const int safeWarnings = qMax(0, warnings);
    if (safeErrors <= 0 && safeWarnings <= 0) {
        return trStatus("No Problems");
    }
    if (safeErrors > 0 && safeWarnings > 0) {
        return trStatus("%1 Errors, %2 Warnings").arg(safeErrors).arg(safeWarnings);
    }
    if (safeErrors > 0) {
        return trStatus("%1 Errors").arg(safeErrors);
    }
    return trStatus("%1 Warnings").arg(safeWarnings);
}
