#include "editor/CompletionPopup.h"

#include "editor/CodeEditor.h"

#include <QAbstractItemView>
#include <QEvent>
#include <QKeyEvent>
#include <QListView>
#include <QTextCursor>
#include <QVBoxLayout>

CompletionPopup::CompletionPopup(QWidget *parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus)
    , m_model(new CompletionModel(this))
    , m_view(new QListView(this))
{
    setObjectName(QStringLiteral("completionPopup"));
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_view->setModel(m_model);
    m_view->setUniformItemSizes(true);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_view);

    resize(420, 240);
    hide();

    connect(m_view, &QListView::activated, this, [this](const QModelIndex &) { activateCurrent(); });
    connect(m_view, &QListView::clicked, this, [this](const QModelIndex &) { activateCurrent(); });
}

void CompletionPopup::showItems(CodeEditor *editor, const QVector<CompletionItem> &items,
                                const QString &prefix)
{
    if (m_editor && m_editor != editor) {
        m_editor->removeEventFilter(this);
    }
    m_editor = editor;
    m_model->setItems(items);
    m_model->setFilter(prefix);
    if (m_model->visibleCount() == 0 && !items.isEmpty()) {
        m_model->setFilter({});
    }
    if (!m_editor || m_model->visibleCount() == 0) {
        hidePopup();
        return;
    }
    m_editor->installEventFilter(this);
    if (m_model->rowCount() > 0) {
        m_view->setCurrentIndex(m_model->index(0, 0));
    }
    placeNearCursor();
    show();
    raise();
}

void CompletionPopup::updateFilter(const QString &prefix)
{
    m_model->setFilter(prefix);
    if (m_model->visibleCount() == 0) {
        hidePopup();
        return;
    }
    m_view->setCurrentIndex(m_model->index(0, 0));
}

void CompletionPopup::hidePopup()
{
    if (m_editor) {
        m_editor->removeEventFilter(this);
        m_editor = nullptr;
    }
    hide();
}

bool CompletionPopup::isVisibleFor(const CodeEditor *editor) const
{
    return isVisible() && m_editor == editor;
}

void CompletionPopup::activateCurrent()
{
    const QModelIndex index = m_view->currentIndex();
    if (!index.isValid()) {
        return;
    }
    const CompletionItem item = m_model->itemAt(index.row());
    hidePopup();
    emit itemActivated(item);
}

void CompletionPopup::placeNearCursor()
{
    if (!m_editor) {
        return;
    }
    const QRect cursor = m_editor->cursorRect();
    const QPoint global = m_editor->mapToGlobal(cursor.bottomLeft());
    move(global + QPoint(0, 4));
}

bool CompletionPopup::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_editor || event->type() != QEvent::KeyPress) {
        return QFrame::eventFilter(watched, event);
    }
    auto *key = static_cast<QKeyEvent *>(event);
    switch (key->key()) {
    case Qt::Key_Escape:
        hidePopup();
        return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Tab:
        activateCurrent();
        return true;
    case Qt::Key_Up: {
        const int row = m_view->currentIndex().row();
        if (row > 0) {
            m_view->setCurrentIndex(m_model->index(row - 1, 0));
        }
        return true;
    }
    case Qt::Key_Down: {
        const int row = m_view->currentIndex().row();
        if (row + 1 < m_model->rowCount()) {
            m_view->setCurrentIndex(m_model->index(row + 1, 0));
        }
        return true;
    }
    default:
        break;
    }
    return QFrame::eventFilter(watched, event);
}
