#include "viewers/FileKind.h"

#include "syntax/LanguageRegistry.h"

#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>

bool isMarkdownPath(const QString &path)
{
    if (path.isEmpty()) {
        return false;
    }
    const QString ext = QFileInfo(path).suffix().toLower();
    return ext == QLatin1String("md") || ext == QLatin1String("markdown")
        || ext == QLatin1String("mdown");
}

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

    if (ext == QLatin1String("svg") || mimeName == QLatin1String("image/svg+xml")) {
        return FileKind::Svg;
    }
    if (ext == QLatin1String("csv") || mimeName == QLatin1String("text/csv")
        || mimeName == QLatin1String("application/csv")) {
        return FileKind::Csv;
    }
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
    if (LanguageRegistry::isExtraLanguagePath(path)) {
        return FileKind::Text;
    }
    return FileKind::Unsupported;
}
