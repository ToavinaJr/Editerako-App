#include "app/MainWindow.h"
#include "core/AppSettings.h"
#include "core/Logging.h"
#include "core/ThemeManager.h"
#include "core/TranslationLoader.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

static bool tryLoadEnvFile(const QString &envPath)
{
    QFile f(envPath);
    if (!f.exists()) {
        return false;
    }
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&f);
    QRegularExpression lineRe("^\\s*([^#=\\s]+)\\s*=\\s*(.*)\\s*$");
    while (!in.atEnd()) {
        QString line = in.readLine();
        QRegularExpressionMatch m = lineRe.match(line);
        if (m.hasMatch()) {
            QByteArray key = m.captured(1).toUtf8();
            QByteArray value = m.captured(2).toUtf8();
            if (value.startsWith('"') && value.endsWith('"') && value.size() >= 2) {
                value = value.mid(1, value.size() - 2);
            } else if (value.startsWith('\'') && value.endsWith('\'') && value.size() >= 2) {
                value = value.mid(1, value.size() - 2);
            }
            qputenv(key.constData(), value);
        }
    }
    return true;
}

static void loadDotEnv(const char *argv0)
{
    QStringList candidates;
    candidates << QDir::current().filePath(".env");

    const QString exePath = QString::fromLocal8Bit(argv0);
    const QFileInfo exeInfo(exePath);
    const QString exeDir = exeInfo.absolutePath();
    candidates << QDir(exeDir).filePath(".env");
    candidates << QDir(exeDir).filePath("../.env");
    candidates << QDir(exeDir).filePath("../../.env");

    for (const QString &path : candidates) {
        if (tryLoadEnvFile(path)) {
            return;
        }
    }
}

int main(int argc, char *argv[])
{
    loadDotEnv(argv[0]);

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Editerako"));
    QCoreApplication::setApplicationName(QStringLiteral("Editerako"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1"));

    AppSettings settings;
    TranslationLoader::install(&app, settings.uiLanguage());
    if (!ThemeManager::apply(app, settings.themeId())) {
        qCWarning(lcCore) << "Falling back without application stylesheet";
    }

    qCInfo(lcCore) << "Starting Editerako";

    MainWindow window;
    window.show();
    return app.exec();
}
