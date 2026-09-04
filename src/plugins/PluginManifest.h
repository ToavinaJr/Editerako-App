#ifndef EDITERAKO_PLUGINMANIFEST_H
#define EDITERAKO_PLUGINMANIFEST_H

#include <QString>
#include <QStringList>
#include <QVector>

struct PluginCommandContribution {
    QString id;
    QString title;
};

struct PluginLanguageContribution {
    QString id;
    QString displayName;
    QStringList extensions;
};

struct PluginViewerContribution {
    QString id;
    QString title;
    QStringList extensions;
};

struct PluginAiContribution {
    QString id;
    QString title;
};

struct PluginPanelContribution {
    QString id;
    QString title;
};

struct PluginManifest {
    QString id;
    QString name;
    QString version;
    int apiVersion = 1;
    QString library;
    QVector<PluginCommandContribution> commands;
    QVector<PluginLanguageContribution> languages;
    QVector<PluginViewerContribution> viewers;
    QVector<PluginAiContribution> aiProviders;
    QVector<PluginPanelContribution> panels;
};

[[nodiscard]] bool isValidPluginId(const QString &id);
[[nodiscard]] PluginManifest parsePluginManifest(const QByteArray &json, QString *error = nullptr);
[[nodiscard]] QString resolvePluginLibraryPath(const QString &pluginDirectory, const QString &library,
                                               QString *error = nullptr);

#endif
