#include "core/KeybindingModel.h"

#include <QSettings>

namespace {

constexpr auto kPrefix = "keybindings/";

} // namespace

KeybindingModel::KeybindingModel()
    : m_ownedSettings(std::make_unique<QSettings>())
    , m_settings(m_ownedSettings.get())
{
}

KeybindingModel::KeybindingModel(QSettings &settings)
    : m_settings(&settings)
{
}

KeybindingModel::~KeybindingModel() = default;

QHash<QString, QKeySequence> KeybindingModel::defaultShortcuts()
{
    QHash<QString, QKeySequence> defaults;
    defaults.insert(QStringLiteral("file.new"), QKeySequence::New);
    defaults.insert(QStringLiteral("file.open"), QKeySequence::Open);
    defaults.insert(QStringLiteral("file.save"), QKeySequence::Save);
    defaults.insert(QStringLiteral("file.saveAs"), QKeySequence::SaveAs);
    defaults.insert(QStringLiteral("file.close"), QKeySequence(QStringLiteral("Ctrl+W")));
    defaults.insert(QStringLiteral("edit.find"), QKeySequence::Find);
    defaults.insert(QStringLiteral("edit.gotoLine"), QKeySequence(QStringLiteral("Ctrl+G")));
    defaults.insert(QStringLiteral("edit.toggleLineComment"), QKeySequence(QStringLiteral("Ctrl+/")));
    defaults.insert(QStringLiteral("edit.toggleBlockComment"), QKeySequence(QStringLiteral("Ctrl+Shift+/")));
    defaults.insert(QStringLiteral("edit.duplicateLine"), QKeySequence(QStringLiteral("Ctrl+Shift+D")));
    defaults.insert(QStringLiteral("edit.deleteLine"), QKeySequence(QStringLiteral("Ctrl+Shift+K")));
    defaults.insert(QStringLiteral("edit.moveLineUp"), QKeySequence(QStringLiteral("Ctrl+Up")));
    defaults.insert(QStringLiteral("edit.moveLineDown"), QKeySequence(QStringLiteral("Ctrl+Down")));
    defaults.insert(QStringLiteral("edit.selectLine"), QKeySequence(QStringLiteral("Ctrl+L")));
    defaults.insert(QStringLiteral("edit.joinLines"), QKeySequence(QStringLiteral("Ctrl+Shift+J")));
    defaults.insert(QStringLiteral("edit.selectNextOccurrence"), QKeySequence(QStringLiteral("Ctrl+D")));
    defaults.insert(QStringLiteral("edit.selectAllOccurrences"), QKeySequence(QStringLiteral("Ctrl+Shift+L")));
    defaults.insert(QStringLiteral("view.terminal"), QKeySequence(QStringLiteral("Ctrl+J")));
    defaults.insert(QStringLiteral("workbench.problems"), QKeySequence(QStringLiteral("Ctrl+Shift+M")));
    defaults.insert(QStringLiteral("workbench.sourceControl"), QKeySequence(QStringLiteral("Ctrl+Shift+G")));
    defaults.insert(QStringLiteral("workbench.commandPalette"), QKeySequence(QStringLiteral("Ctrl+Shift+P")));
    defaults.insert(QStringLiteral("workbench.quickOpen"), QKeySequence(QStringLiteral("Ctrl+P")));
    defaults.insert(QStringLiteral("workbench.search"), QKeySequence(QStringLiteral("Ctrl+Shift+F")));
    defaults.insert(QStringLiteral("preferences.open"), QKeySequence(QStringLiteral("Ctrl+,")));
    defaults.insert(QStringLiteral("editor.triggerSuggest"), QKeySequence(QStringLiteral("Ctrl+Space")));
    defaults.insert(QStringLiteral("editor.gotoDefinition"), QKeySequence(QStringLiteral("F12")));
    defaults.insert(QStringLiteral("editor.findReferences"), QKeySequence(QStringLiteral("Shift+F12")));
    defaults.insert(QStringLiteral("editor.renameSymbol"), QKeySequence(QStringLiteral("F2")));
    defaults.insert(QStringLiteral("editor.documentSymbols"), QKeySequence(QStringLiteral("Ctrl+Shift+O")));
    defaults.insert(QStringLiteral("editor.workspaceSymbols"), QKeySequence(QStringLiteral("Ctrl+T")));
    return defaults;
}

QString KeybindingModel::settingsKey(const QString &commandId) const
{
    return QLatin1String(kPrefix) + commandId;
}

QKeySequence KeybindingModel::shortcut(const QString &commandId) const
{
    if (!m_settings) {
        return defaultShortcuts().value(commandId);
    }
    const QString key = settingsKey(commandId);
    if (!m_settings->contains(key)) {
        return defaultShortcuts().value(commandId);
    }
    const QString stored = m_settings->value(key).toString();
    if (stored.isEmpty()) {
        return {};
    }
    return QKeySequence::fromString(stored, QKeySequence::PortableText);
}

QString KeybindingModel::conflictId(const QKeySequence &sequence, const QString &exceptCommandId) const
{
    if (sequence.isEmpty()) {
        return {};
    }

    QStringList ids = defaultShortcuts().keys();
    if (m_settings) {
        m_settings->beginGroup(QString::fromLatin1(kPrefix).chopped(1));
        ids.append(m_settings->childKeys());
        m_settings->endGroup();
    }
    ids.removeDuplicates();

    for (const QString &id : ids) {
        if (id == exceptCommandId) {
            continue;
        }
        if (shortcut(id) == sequence) {
            return id;
        }
    }
    return {};
}

bool KeybindingModel::setShortcut(const QString &commandId, const QKeySequence &sequence,
                                  QString *conflictIdOut)
{
    if (commandId.isEmpty() || !m_settings) {
        return false;
    }
    const QString conflict = conflictId(sequence, commandId);
    if (!conflict.isEmpty()) {
        if (conflictIdOut) {
            *conflictIdOut = conflict;
        }
        return false;
    }

    m_settings->setValue(settingsKey(commandId), sequence.toString(QKeySequence::PortableText));
    m_settings->sync();
    return true;
}

void KeybindingModel::resetToDefault(const QString &commandId)
{
    if (!m_settings || commandId.isEmpty()) {
        return;
    }
    m_settings->remove(settingsKey(commandId));
    m_settings->sync();
}

QStringList KeybindingModel::overriddenCommandIds() const
{
    if (!m_settings) {
        return {};
    }
    m_settings->beginGroup(QString::fromLatin1(kPrefix).chopped(1));
    const QStringList ids = m_settings->childKeys();
    m_settings->endGroup();
    return ids;
}
