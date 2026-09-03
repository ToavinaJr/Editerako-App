#ifndef EDITERAKO_KEYBINDINGEDITOR_H
#define EDITERAKO_KEYBINDINGEDITOR_H

#include <QWidget>

class CommandRegistry;
class KeybindingManager;
class QKeySequenceEdit;
class QLabel;
class QTableWidget;

class KeybindingEditor : public QWidget
{
    Q_OBJECT

public:
    KeybindingEditor(KeybindingManager *manager, CommandRegistry *registry, QWidget *parent = nullptr);

    void reload();

private:
    void populate();
    void assignCurrent();
    void resetCurrent();
    [[nodiscard]] QString selectedCommandId() const;

    KeybindingManager *m_manager = nullptr;
    CommandRegistry *m_registry = nullptr;
    QTableWidget *m_table = nullptr;
    QKeySequenceEdit *m_sequenceEdit = nullptr;
    QLabel *m_status = nullptr;
};

#endif
