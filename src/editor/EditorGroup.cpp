#include "editor/EditorGroup.h"

#include <QEvent>
#include <QMouseEvent>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace {

constexpr auto kPinned = "tabPinned";
constexpr auto kPreview = "tabPreview";

QWidget *tabWidgetAt(QTabWidget *tabs, int index)
{
    if (!tabs || index < 0 || index >= tabs->count()) {
        return nullptr;
    }
    return tabs->widget(index);
}

} // namespace

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
    connect(m_tabs->tabBar(), &QTabBar::tabMoved, this, [this](int, int) { onTabMoved(); });
    connect(m_tabs->tabBar(), &QTabBar::customContextMenuRequested, this, [this](const QPoint &pos) {
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

bool EditorGroup::isPinned(int index) const
{
    QWidget *widget = tabWidgetAt(m_tabs, index);
    return widget && widget->property(kPinned).toBool();
}

void EditorGroup::setPinned(int index, bool pinned)
{
    QWidget *widget = tabWidgetAt(m_tabs, index);
    if (!widget) {
        return;
    }
    widget->setProperty(kPinned, pinned);
    if (pinned) {
        widget->setProperty(kPreview, false);
    }
    enforcePinOrder();
}

bool EditorGroup::isPreview(int index) const
{
    QWidget *widget = tabWidgetAt(m_tabs, index);
    return widget && widget->property(kPreview).toBool();
}

void EditorGroup::setPreview(int index, bool preview)
{
    QWidget *widget = tabWidgetAt(m_tabs, index);
    if (!widget) {
        return;
    }
    if (preview) {
        for (int i = 0; i < m_tabs->count(); ++i) {
            if (QWidget *other = m_tabs->widget(i)) {
                other->setProperty(kPreview, other == widget);
            }
        }
        widget->setProperty(kPinned, false);
    } else {
        widget->setProperty(kPreview, false);
    }
}

void EditorGroup::promote(int index)
{
    setPreview(index, false);
}

int EditorGroup::previewIndex() const
{
    for (int i = 0; i < m_tabs->count(); ++i) {
        if (isPreview(i)) {
            return i;
        }
    }
    return -1;
}

int EditorGroup::pinnedCount() const
{
    int count = 0;
    for (int i = 0; i < m_tabs->count(); ++i) {
        if (isPinned(i)) {
            ++count;
        }
    }
    return count;
}

void EditorGroup::enforcePinOrder()
{
    if (m_reordering || !m_tabs) {
        return;
    }
    m_reordering = true;
    QTabBar *bar = m_tabs->tabBar();
    int firstUnpinned = -1;
    for (int i = 0; i < m_tabs->count(); ++i) {
        if (!isPinned(i)) {
            firstUnpinned = i;
            break;
        }
    }
    if (firstUnpinned >= 0) {
        for (int i = m_tabs->count() - 1; i > firstUnpinned; --i) {
            if (isPinned(i)) {
                bar->moveTab(i, firstUnpinned);
                firstUnpinned++;
            }
        }
    }
    m_reordering = false;
}

void EditorGroup::onTabMoved()
{
    if (!m_reordering) {
        enforcePinOrder();
    }
}

bool EditorGroup::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_tabs->tabBar() && event->type() == QEvent::MouseButtonPress) {
        const auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() == Qt::MiddleButton) {
            const int index = m_tabs->tabBar()->tabAt(mouse->pos());
            if (index >= 0 && !isPinned(index)) {
                emit tabCloseRequested(index);
            }
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
