#ifndef EDITERAKO_SOURCECONTROLPANEL_H
#define EDITERAKO_SOURCECONTROLPANEL_H

#include "scm/SourceControlTypes.h"

#include <QWidget>

class GitCliProvider;
class QLabel;
class QLineEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

class SourceControlPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SourceControlPanel(GitCliProvider *provider, QWidget *parent = nullptr);
    void setWorkspace(const QString &path);

signals:
    void diffRequested(const QString &path, bool staged);

private:
    void rebuild(const ScmStatus &status);
    void updateActions();
    [[nodiscard]] QString selectedPath() const;
    [[nodiscard]] bool selectedStaged() const;

    GitCliProvider *m_provider = nullptr;
    QLabel *m_header = nullptr;
    QLineEdit *m_message = nullptr;
    QTreeWidget *m_tree = nullptr;
    QPushButton *m_stage = nullptr;
    QPushButton *m_unstage = nullptr;
    QPushButton *m_discard = nullptr;
};

#endif

