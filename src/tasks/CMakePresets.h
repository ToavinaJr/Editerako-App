#ifndef EDITERAKO_CMAKEPRESETS_H
#define EDITERAKO_CMAKEPRESETS_H

#include <QString>
#include <QStringList>
#include <QVector>

struct CMakePresetInfo {
    QString name;
    QString displayName;
    QString binaryDir;
    QString inherits;
    bool hidden = false;
};

struct CMakeWorkspace {
    bool detected = false;
    QString sourceDir;
    QString cmakeExecutable;
    QString ctestExecutable;
    QVector<CMakePresetInfo> configurePresets;

    [[nodiscard]] QStringList visiblePresetNames() const;
    [[nodiscard]] QString binaryDirFor(const QString &presetName) const;
};

[[nodiscard]] QString expandCMakeMacro(const QString &value, const QString &sourceDir,
                                       const QString &presetName);
[[nodiscard]] QVector<CMakePresetInfo> parseConfigurePresets(const QByteArray &json);
[[nodiscard]] CMakeWorkspace inspectCMakeWorkspace(const QString &workspaceRoot);

#endif
