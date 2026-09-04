#ifndef EDITERAKO_TASKSPANEL_H
#define EDITERAKO_TASKSPANEL_H

#include <QWidget>

class TaskManager;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

class TasksPanel : public QWidget
{
    Q_OBJECT

public:
    explicit TasksPanel(TaskManager *manager, QWidget *parent = nullptr);
    void setWorkspace(const QString &path);

private:
    void rebuild();
    void updateRunning();
    void runSelected();

    TaskManager *m_manager = nullptr;
    QLabel *m_header = nullptr;
    QComboBox *m_preset = nullptr;
    QLineEdit *m_target = nullptr;
    QListWidget *m_list = nullptr;
    QPushButton *m_run = nullptr;
    QPushButton *m_cancel = nullptr;
    QPushButton *m_configure = nullptr;
    QPushButton *m_build = nullptr;
    QPushButton *m_clean = nullptr;
    QPushButton *m_test = nullptr;
    QPushButton *m_launch = nullptr;
};

#endif
