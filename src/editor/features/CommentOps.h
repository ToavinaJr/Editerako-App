#ifndef EDITERAKO_COMMENTOPS_H
#define EDITERAKO_COMMENTOPS_H

#include <QString>
#include <QStringList>

[[nodiscard]] QStringList toggleLineComments(QStringList lines, const QString &marker);
[[nodiscard]] QString toggleBlockComment(const QString &text,
                                         const QString &open,
                                         const QString &close);

#endif
