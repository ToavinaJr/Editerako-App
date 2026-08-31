#include "core/DropPaths.h"

#include <QMimeData>
#include <QUrl>

QStringList localPathsFromMimeData(const QMimeData *mime)
{
    QStringList paths;
    if (!mime || !mime->hasUrls()) {
        return paths;
    }

    const QList<QUrl> urls = mime->urls();
    for (const QUrl &url : urls) {
        QString filePath = url.toLocalFile();

#ifdef Q_OS_WIN
        if (filePath.isEmpty() || filePath.startsWith(QLatin1String("file://"))) {
            QString path = url.path();
            if (path.startsWith(QLatin1Char('/'))) {
                path = path.mid(1);
            }
            filePath = path;
        }
#endif

        if (!filePath.isEmpty()) {
            paths.append(filePath);
        }
    }
    return paths;
}
