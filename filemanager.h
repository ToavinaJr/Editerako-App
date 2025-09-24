#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>

class filemanager : public QObject
{
    Q_OBJECT
public:
    explicit filemanager(QObject *parent = nullptr);

signals:
};

#endif // FILEMANAGER_H
