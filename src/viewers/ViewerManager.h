#ifndef EDITERAKO_VIEWERMANAGER_H
#define EDITERAKO_VIEWERMANAGER_H

#include "viewers/FileKind.h"

#include <QList>
#include <QObject>
#include <QString>

class EditorManager;
class IFileViewerProvider;

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
    OpenResult open(const QString &filePath, bool preview = false);
    OpenResult openNew(const QString &filePath, bool preview = false);
    OpenResult openMarkdownPreview(const QString &filePath);

    void addProvider(IFileViewerProvider *provider);
    void removeProvider(IFileViewerProvider *provider);

private:
    OpenResult openPdf(const QString &filePath, bool preview);
    OpenResult openImage(const QString &filePath, bool preview);
    OpenResult openSvg(const QString &filePath, bool preview);
    OpenResult openCsv(const QString &filePath, bool preview);
    OpenResult openWithProvider(IFileViewerProvider *provider, const QString &filePath, bool preview);

    EditorManager *m_editors = nullptr;
    QList<IFileViewerProvider *> m_providers;
};

#endif
