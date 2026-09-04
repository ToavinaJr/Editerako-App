#ifndef EDITERAKO_LAUNCHFILE_H
#define EDITERAKO_LAUNCHFILE_H

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

struct LaunchContext {
    QString workspaceFolder;
    QString file;
};

struct LaunchConfiguration {
    QString name;
    QString type;
    QString request;
    QString adapterCommand;
    QStringList adapterArgs;
    QJsonObject arguments;
};

[[nodiscard]] QString launchFilePath(const QString &workspaceRoot);
[[nodiscard]] QString expandLaunchVariables(const QString &text, const LaunchContext &context);
[[nodiscard]] QJsonValue expandLaunchValue(const QJsonValue &value, const LaunchContext &context);
[[nodiscard]] LaunchConfiguration expandLaunchConfiguration(LaunchConfiguration config,
                                                            const LaunchContext &context);
[[nodiscard]] bool resolveDebugAdapter(LaunchConfiguration *config, QString *error = nullptr);
[[nodiscard]] QVector<LaunchConfiguration> parseLaunchJson(const QByteArray &json,
                                                           QString *error = nullptr);
[[nodiscard]] QVector<LaunchConfiguration> loadLaunchFile(const QString &workspaceRoot,
                                                          QString *error = nullptr);
[[nodiscard]] QByteArray defaultLaunchTemplateJson();
bool writeLaunchTemplate(const QString &workspaceRoot, QString *error = nullptr);

#endif
