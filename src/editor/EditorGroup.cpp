#include "editor/EditorGroup.h"

#include <QEvent>
#include <QMouseEvent>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

EditorGroup::EditorGroup(QWidget *parent)
    : QWidget(parent)
    , m_tabs(new QTabWidget(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_tabs);

    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
    m_tabs->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tabs->tabBar()->installEventFilter(this);

    connect(m_tabs, &QTabWidget::currentChanged, this, &EditorGroup::currentChanged);
    connect(m_tabs, &QTabWidget::tabCloseRequested, this, &EditorGroup::tabCloseRequested);
    connect(m_tabs->tabBar(), &QTabBar::customContextMenuRequested, this,
            [this](const QPoint &pos) {
                const int index = m_tabs->tabBar()->tabAt(pos);
                emit tabContextMenuRequested(index, m_tabs->tabBar()->mapToGlobal(pos));
            });
}

void EditorGroup::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    m_active = active;
    m_tabs->setProperty("editorGroupActive", active);
    m_tabs->style()->unpolish(m_tabs);
    m_tabs->style()->polish(m_tabs);
}

bool EditorGroup::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_tabs->tabBar() && event->type() == QEvent::MouseButtonPress) {
        const auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() == Qt::MiddleButton) {
            const int index = m_tabs->tabBar()->tabAt(mouse->pos());
            if (index >= 0) {
                emit tabCloseRequested(index);
            }
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
