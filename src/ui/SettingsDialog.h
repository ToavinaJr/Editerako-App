#ifndef EDITERAKO_SETTINGSDIALOG_H
#define EDITERAKO_SETTINGSDIALOG_H

#include <QDialog>

class CommandRegistry;
class KeybindingManager;
class KeybindingEditor;
class QCheckBox;
class QComboBox;
class QFontComboBox;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(KeybindingManager *keybindings, CommandRegistry *commands, QWidget *parent = nullptr);

signals:
    void settingsApplied();

private:
    void load();
    void save();
    void applyClicked();

    QComboBox *m_theme = nullptr;
    QCheckBox *m_autoSave = nullptr;
    QSpinBox *m_autoSaveDelay = nullptr;
    QFontComboBox *m_fontFamily = nullptr;
    QSpinBox *m_fontSize = nullptr;
    QSpinBox *m_tabSize = nullptr;
    QCheckBox *m_insertSpaces = nullptr;
    QCheckBox *m_wordWrap = nullptr;
    QCheckBox *m_lineNumbers = nullptr;
    QSpinBox *m_warnMb = nullptr;
    QSpinBox *m_syntaxMb = nullptr;
    QPlainTextEdit *m_excluded = nullptr;
    QLineEdit *m_shell = nullptr;
    QComboBox *m_aiProvider = nullptr;
    QLineEdit *m_aiModel = nullptr;
    QLineEdit *m_aiEndpoint = nullptr;
    KeybindingEditor *m_keybindings = nullptr;
};

#endif
