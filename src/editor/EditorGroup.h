#ifndef EDITERAKO_EDITORGROUP_H
#define EDITERAKO_EDITORGROUP_H

#include <QWidget>

class QTabWidget;

class EditorGroup : public QWidget
{
    Q_OBJECT

public:
    explicit EditorGroup(QWidget *parent = nullptr);

    [[nodiscard]] QTabWidget *tabWidget() const { return m_tabs; }
    void setActive(bool active);
    [[nodiscard]] bool isActive() const { return m_active; }

signals:
    void currentChanged();
    void tabCloseRequested(int index);
    void tabContextMenuRequested(int index, const QPoint &globalPos);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QTabWidget *m_tabs = nullptr;
    bool m_active = false;
};

#endif
