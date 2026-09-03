#ifndef EDITERAKO_PROBLEMSPANEL_H
#define EDITERAKO_PROBLEMSPANEL_H

#include "editor/ProblemModel.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QTreeWidget;
class QTreeWidgetItem;

class ProblemsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ProblemsPanel(QWidget *parent = nullptr);

    [[nodiscard]] ProblemModel *model() const { return m_model; }
    void setWorkspaceRoot(const QString &root);

signals:
    void problemActivated(const QString &path, int line, int column);

private:
    void rebuild();
    void activateItem(QTreeWidgetItem *item);
    [[nodiscard]] QString displayPath(const QString &path) const;

    ProblemModel *m_model = nullptr;
    QComboBox *m_filter = nullptr;
    QLabel *m_summary = nullptr;
    QTreeWidget *m_tree = nullptr;
    QString m_workspaceRoot;
};

#endif
