#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDir>
#include <QFileInfo>

// Try to load .env from a specific path and set environment variables
static bool tryLoadEnvFile(const QString &envPath)
{
    QFile f(envPath);
    if (!f.exists()) return false;
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QTextStream in(&f);
    QRegularExpression lineRe("^\\s*([^#=\\s]+)\\s*=\\s*(.*)\\s*$");
    while (!in.atEnd()) {
        QString line = in.readLine();
        QRegularExpressionMatch m = lineRe.match(line);
        if (m.hasMatch()) {
            QByteArray key = m.captured(1).toUtf8();
            QByteArray value = m.captured(2).toUtf8();
            // Trim any surrounding quotes
            if (value.startsWith('"') && value.endsWith('"') && value.size() >= 2) {
                value = value.mid(1, value.size()-2);
            } else if (value.startsWith('\'') && value.endsWith('\'') && value.size() >= 2) {
                value = value.mid(1, value.size()-2);
            }
            qputenv(key.constData(), value);
        }
    }
    f.close();
    return true;
}

static void loadDotEnv(const char *argv0)
{
    // Strategy: try multiple paths to find .env
    // 1) Current working directory
    // 2) Directory containing the executable
    // 3) Parent of executable dir (for build/Debug layout)

    QStringList candidates;
    candidates << QDir::current().filePath(".env");

    // Use argv[0] to derive executable directory (QCoreApplication not yet created)
    QString exePath = QString::fromLocal8Bit(argv0);
    QFileInfo exeInfo(exePath);
    QString exeDir = exeInfo.absolutePath();
    candidates << QDir(exeDir).filePath(".env");
    // Parent directory (common when build output is in a subfolder)
    candidates << QDir(exeDir).filePath("../.env");
    candidates << QDir(exeDir).filePath("../../.env");

    for (const QString &path : candidates) {
        if (tryLoadEnvFile(path)) {
            return; // stop on first success
        }
    }
}

int main(int argc, char *argv[])
{
    // Load .env before creating QApplication (so GEMINI_API_KEY available to qgetenv)
    loadDotEnv(argv[0]);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
