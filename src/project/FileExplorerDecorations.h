#ifndef EDITERAKO_FILEEXPLORERDECORATIONS_H
#define EDITERAKO_FILEEXPLORERDECORATIONS_H

#include <QColor>
#include <QString>

[[nodiscard]] QColor fileExplorerBadgeColor(const QString &badge);
[[nodiscard]] QString fileExplorerFileIcon(const QString &fileName);
[[nodiscard]] QString fileExplorerItemText(const QString &icon, const QString &name,
                                           const QString &badge);

#endif
