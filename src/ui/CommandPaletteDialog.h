#ifndef EDITERAKO_COMMANDPALETTEDIALOG_H
#define EDITERAKO_COMMANDPALETTEDIALOG_H

#include "ui/FuzzyPickerDialog.h"

class CommandRegistry;

class CommandPaletteDialog : public FuzzyPickerDialog
{
    Q_OBJECT

public:
    explicit CommandPaletteDialog(CommandRegistry *registry, QWidget *parent = nullptr);
};

#endif
