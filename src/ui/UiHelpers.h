#ifndef EDITERAKO_UIHELPERS_H
#define EDITERAKO_UIHELPERS_H

#include <QString>

class QWidget;

void replacePlaceholder(QWidget *placeholder, QWidget *replacement, QWidget *fallbackParent);

QString promptText(QWidget *parent,
                   const QString &title,
                   const QString &label,
                   const QString &defaultValue,
                   int minWidth,
                   int minHeight);

#endif
