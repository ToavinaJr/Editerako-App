#ifndef EDITERAKO_KEYBINDINGMANAGER_H
#define EDITERAKO_KEYBINDINGMANAGER_H

#include "core/KeybindingModel.h"

#include <QObject>
#include <QKeySequence>
#include <QString>

class CommandRegistry;
class QSettings;

class KeybindingManager : public QObject
{
    Q_OBJECT

public:
    explicit KeybindingManager(CommandRegistry *registry, QObject *parent = nullptr);
    KeybindingManager(CommandRegistry *registry, QSettings &settings, QObject *parent = nullptr);

    [[nodiscard]] KeybindingModel &model() { return m_model; }
    [[nodiscard]] const KeybindingModel &model() const { return m_model; }

    void apply();
    bool setShortcut(const QString &commandId, const QKeySequence &sequence,
                     QString *conflictId = nullptr);
    void resetShortcut(const QString &commandId);

    [[nodiscard]] QKeySequence shortcut(const QString &commandId) const;

signals:
    void shortcutsChanged();

private:
    void applyOne(const QString &commandId);

    CommandRegistry *m_registry = nullptr;
    KeybindingModel m_model;
};

#endif
