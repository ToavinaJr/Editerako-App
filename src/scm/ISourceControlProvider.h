#ifndef EDITERAKO_ISOURCECONTROLPROVIDER_H
#define EDITERAKO_ISOURCECONTROLPROVIDER_H

#include "scm/SourceControlTypes.h"

#include <QObject>

class ISourceControlProvider : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    ~ISourceControlProvider() override = default;

    virtual void setWorkspace(const QString &path) = 0;
    [[nodiscard]] virtual QString workspace() const = 0;
    virtual void refresh() = 0;
    virtual void stage(const QStringList &paths) = 0;
    virtual void unstage(const QStringList &paths) = 0;
    virtual void discard(const QStringList &paths) = 0;
    virtual void commit(const QString &message) = 0;
    virtual void requestDiff(const QString &path, bool staged) = 0;

signals:
    void statusChanged(const ScmStatus &status);
    void diffReady(const QString &path, const QString &diff);
    void operationFailed(const ScmError &error);
    void busyChanged(bool busy);
};

#endif

