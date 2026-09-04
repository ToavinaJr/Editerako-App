#ifndef EDITERAKO_IFILEVIEWERPROVIDER_H
#define EDITERAKO_IFILEVIEWERPROVIDER_H

#include <QString>

class QWidget;

class IFileViewerProvider
{
public:
    virtual ~IFileViewerProvider() = default;

    [[nodiscard]] virtual QString id() const = 0;
    [[nodiscard]] virtual bool canOpen(const QString &path) const = 0;
    virtual QWidget *create(const QString &path, QWidget *parent, QString *error) = 0;
};

#endif
