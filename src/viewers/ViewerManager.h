#ifndef EDITERAKO_VIEWERMANAGER_H
#define EDITERAKO_VIEWERMANAGER_H

#include "viewers/FileKind.h"

#include <QObject>
#include <QString>

class EditorManager;

class ViewerManager : public QObject
{
    Q_OBJECT

public:
    using FileKind = ::FileKind;

    enum class OpenResult {
        Opened,
        Unsupported,
        Failed,
    };

    explicit ViewerManager(EditorManager *editors, QObject *parent = nullptr);

    [[nodiscard]] static FileKind kindForPath(const QString &path);
    OpenResult open(const QString &filePath);
    OpenResult openNew(const QString &filePath);

private:
    OpenResult openPdf(const QString &filePath);
    OpenResult openImage(const QString &filePath);

    EditorManager *m_editors = nullptr;
};

#endif
