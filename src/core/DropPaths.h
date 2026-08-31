#ifndef EDITERAKO_DROPPATHS_H
#define EDITERAKO_DROPPATHS_H

#include <QStringList>

class QMimeData;

QStringList localPathsFromMimeData(const QMimeData *mime);

#endif
