#include "project/FileExplorerDecorations.h"

#include <QFileInfo>

QColor fileExplorerBadgeColor(const QString &badge)
{
    if (badge == QLatin1String("M") || badge == QLatin1String("R") || badge == QLatin1String("C")) {
        return QColor(QStringLiteral("#d29922"));
    }
    if (badge == QLatin1String("A") || badge == QLatin1String("U")) {
        return QColor(QStringLiteral("#3fb950"));
    }
    if (badge == QLatin1String("D") || badge == QLatin1String("!")) {
        return QColor(QStringLiteral("#f85149"));
    }
    return QColor(QStringLiteral("#8b949e"));
}

QString fileExplorerFileIcon(const QString &fileName)
{
    const QString ext = QFileInfo(fileName).suffix().toLower();
    if (ext == QLatin1String("cpp") || ext == QLatin1String("cxx") || ext == QLatin1String("cc")
        || ext == QLatin1String("c")) {
        return QStringLiteral("🔵");
    }
    if (ext == QLatin1String("h") || ext == QLatin1String("hpp") || ext == QLatin1String("hxx")) {
        return QStringLiteral("🟦");
    }
    if (ext == QLatin1String("py")) {
        return QStringLiteral("🐍");
    }
    if (ext == QLatin1String("js")) {
        return QStringLiteral("🟨");
    }
    if (ext == QLatin1String("html") || ext == QLatin1String("htm")) {
        return QStringLiteral("🌐");
    }
    if (ext == QLatin1String("css")) {
        return QStringLiteral("🎨");
    }
    if (ext == QLatin1String("php")) {
        return QStringLiteral("🐘");
    }
    if (ext == QLatin1String("txt")) {
        return QStringLiteral("📝");
    }
    if (ext == QLatin1String("json")) {
        return QStringLiteral("📋");
    }
    if (ext == QLatin1String("xml") || ext == QLatin1String("ui")) {
        return QStringLiteral("📄");
    }
    if (ext == QLatin1String("exe") || ext == QLatin1String("bin")) {
        return QStringLiteral("⚙️");
    }
    return QStringLiteral("📄");
}

QString fileExplorerItemText(const QString &icon, const QString &name, const QString &badge)
{
    if (badge.isEmpty()) {
        return QStringLiteral("%1 %2").arg(icon, name);
    }
    return QStringLiteral("%1 %2  %3").arg(icon, name, badge);
}
