#ifndef EDITERAKO_QUICKOPENDIALOG_H
#define EDITERAKO_QUICKOPENDIALOG_H

#include "ui/FuzzyPickerDialog.h"

#include <QStringList>

class WorkspaceFileIndex;

class QuickOpenDialog : public FuzzyPickerDialog
{
    Q_OBJECT

public:
    QuickOpenDialog(WorkspaceFileIndex *index,
                    const QStringList &additionalPaths,
                    QWidget *parent = nullptr);

    [[nodiscard]] QString selectedPath() const;
    [[nodiscard]] int selectedLine() const;

protected:
    [[nodiscard]] QString rankQuery() const override;

private:
    void reloadItems();

    WorkspaceFileIndex *m_index = nullptr;
    QStringList m_additionalPaths;
};

#endif
