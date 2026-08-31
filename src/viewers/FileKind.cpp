#include "viewers/FileKind.h"

#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>

FileKind fileKindForPath(const QString &path)
{
    if (path.isEmpty()) {
        return FileKind::Unsupported;
    }

    const QFileInfo info(path);
    const QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(info);
    const QString mimeName = mime.name();
    const QString ext = info.suffix().toLower();

    if (mimeName.startsWith(QLatin1String("text/"))
        || mimeName.contains(QLatin1String("json"))
        || mimeName.contains(QLatin1String("xml"))
        || mimeName.contains(QLatin1String("html"))
        || ext == QLatin1String("tsx")) {
        return FileKind::Text;
    }
    if (mimeName == QLatin1String("application/pdf")) {
        return FileKind::Pdf;
    }
    if (mimeName.startsWith(QLatin1String("image/"))) {
        return FileKind::Image;
    }
    return FileKind::Unsupported;
}
