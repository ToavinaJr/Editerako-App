#ifndef EDITERAKO_KEYBINDINGMODEL_H
#define EDITERAKO_KEYBINDINGMODEL_H

#include <QHash>
#include <QKeySequence>
#include <QString>
#include <QStringList>
#include <memory>

class QSettings;

class KeybindingModel
{
public:
    KeybindingModel();
    explicit KeybindingModel(QSettings &settings);
    ~KeybindingModel();

    KeybindingModel(const KeybindingModel &) = delete;
    KeybindingModel &operator=(const KeybindingModel &) = delete;

    [[nodiscard]] static QHash<QString, QKeySequence> defaultShortcuts();

    [[nodiscard]] QKeySequence shortcut(const QString &commandId) const;
    [[nodiscard]] QString conflictId(const QKeySequence &sequence,
                                     const QString &exceptCommandId = {}) const;

    bool setShortcut(const QString &commandId, const QKeySequence &sequence,
                     QString *conflictIdOut = nullptr);
    void resetToDefault(const QString &commandId);

    [[nodiscard]] QStringList overriddenCommandIds() const;

private:
    [[nodiscard]] QString settingsKey(const QString &commandId) const;
    [[nodiscard]] QSettings *settings() const { return m_settings; }

    std::unique_ptr<QSettings> m_ownedSettings;
    QSettings *m_settings = nullptr;
};

#endif
