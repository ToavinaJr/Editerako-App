#include "ui/CommandPaletteDialog.h"

#include "core/CommandRegistry.h"

#include <QAction>
#include <QKeySequence>

CommandPaletteDialog::CommandPaletteDialog(CommandRegistry *registry, QWidget *parent)
    : FuzzyPickerDialog(parent)
{
    setWindowTitle(tr("Command Palette"));
    setPlaceholderText(tr("Type a command"));
    setObjectName(QStringLiteral("commandPaletteDialog"));

    QList<FuzzyPickerItem> items;
    if (!registry) {
        setItems(items);
        return;
    }

    const QStringList ids = registry->ids();
    items.reserve(ids.size());
    for (const QString &id : ids) {
        QAction *action = registry->action(id);
        if (!action) {
            continue;
        }
        FuzzyPickerItem item;
        item.id = id;
        item.display = action->text().remove(QLatin1Char('&'));
        item.filterText = item.display + QLatin1Char(' ') + id;
        item.hint = action->shortcut().toString(QKeySequence::NativeText);
        item.enabled = action->isEnabled();
        items.append(item);
    }
    setItems(items);
}
