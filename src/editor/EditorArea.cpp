#include "editor/EditorArea.h"

#include "editor/EditorGroup.h"

#include <QSplitter>
#include <QVBoxLayout>

EditorArea::EditorArea(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
}

void EditorArea::setInitialGroup(EditorGroup *group)
{
    if (!group) {
        return;
    }
    while (QLayoutItem *item = m_layout->takeAt(0)) {
        delete item;
    }
    m_layout->addWidget(group);
}

QWidget *EditorArea::rootWidget() const
{
    if (m_layout->count() < 1) {
        return nullptr;
    }
    return m_layout->itemAt(0)->widget();
}

void EditorArea::collectGroups(QWidget *node, QList<EditorGroup *> *out) const
{
    if (!node || !out) {
        return;
    }
    if (auto *group = qobject_cast<EditorGroup *>(node)) {
        out->append(group);
        return;
    }
    if (auto *splitter = qobject_cast<QSplitter *>(node)) {
        for (int i = 0; i < splitter->count(); ++i) {
            collectGroups(splitter->widget(i), out);
        }
    }
}

QList<EditorGroup *> EditorArea::groups() const
{
    QList<EditorGroup *> result;
    collectGroups(rootWidget(), &result);
    return result;
}

void EditorArea::split(EditorGroup *existing, EditorGroup *added, Qt::Orientation orientation)
{
    if (!existing || !added) {
        return;
    }

    QWidget *parent = existing->parentWidget();
    if (auto *splitter = qobject_cast<QSplitter *>(parent)) {
        if (splitter->orientation() == orientation) {
            const int index = splitter->indexOf(existing);
            splitter->insertWidget(index + 1, added);
            QList<int> sizes = splitter->sizes();
            if (!sizes.isEmpty()) {
                splitter->setSizes(QList<int>(splitter->count(), 1000));
            }
            return;
        }

        auto *inner = new QSplitter(orientation, splitter);
        const int index = splitter->indexOf(existing);
        splitter->insertWidget(index, inner);
        inner->addWidget(existing);
        inner->addWidget(added);
        inner->setSizes({1000, 1000});
        return;
    }

    auto *splitter = new QSplitter(orientation, this);
    m_layout->removeWidget(existing);
    m_layout->addWidget(splitter);
    splitter->addWidget(existing);
    splitter->addWidget(added);
    splitter->setSizes({1000, 1000});
}

void EditorArea::unwrapSplitter(QSplitter *splitter)
{
    if (!splitter || splitter->count() != 1) {
        return;
    }

    QWidget *remain = splitter->widget(0);
    QWidget *parent = splitter->parentWidget();
    if (auto *outer = qobject_cast<QSplitter *>(parent)) {
        const int index = outer->indexOf(splitter);
        remain->setParent(nullptr);
        outer->insertWidget(index, remain);
        splitter->hide();
        splitter->deleteLater();
        return;
    }

    m_layout->removeWidget(splitter);
    remain->setParent(nullptr);
    m_layout->addWidget(remain);
    splitter->hide();
    splitter->deleteLater();
}

void EditorArea::removeGroup(EditorGroup *group)
{
    if (!group || groups().size() <= 1) {
        return;
    }

    QWidget *parent = group->parentWidget();
    auto *splitter = qobject_cast<QSplitter *>(parent);
    group->hide();
    group->setParent(nullptr);
    group->deleteLater();
    if (splitter) {
        unwrapSplitter(splitter);
    }
}
