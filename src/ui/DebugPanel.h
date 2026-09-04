#ifndef EDITERAKO_DEBUGPANEL_H
#define EDITERAKO_DEBUGPANEL_H

#include "debug/DapTypes.h"

#include <QWidget>

class DebugSession;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

class DebugPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DebugPanel(DebugSession *session, QWidget *parent = nullptr);

private:
    void rebuildConfigurations();
    void updateButtons();
    void setStackFrames(const QVector<DapStackFrame> &frames);
    void setScopes(const QVector<DapScope> &scopes);
    void addVariables(int variablesReference, const QVector<DapVariable> &variables);
    void appendOutput(const QString &category, const QString &text);
    void submitEvaluate();
    void onStackActivated();
    void onVariableExpanded(QTreeWidgetItem *item);

    DebugSession *m_session = nullptr;
    QComboBox *m_configs = nullptr;
    QPushButton *m_start = nullptr;
    QPushButton *m_stop = nullptr;
    QPushButton *m_continue = nullptr;
    QPushButton *m_pause = nullptr;
    QPushButton *m_stepOver = nullptr;
    QPushButton *m_stepInto = nullptr;
    QPushButton *m_stepOut = nullptr;
    QPushButton *m_createLaunch = nullptr;
    QTreeWidget *m_stack = nullptr;
    QTreeWidget *m_variables = nullptr;
    QPlainTextEdit *m_console = nullptr;
    QLineEdit *m_repl = nullptr;
};

#endif
