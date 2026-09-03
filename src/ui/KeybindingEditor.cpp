#include "ui/KeybindingEditor.h"

#include "core/CommandRegistry.h"
#include "core/KeybindingManager.h"

#include <QAbstractItemView>
#include <QAction>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

KeybindingEditor::KeybindingEditor(KeybindingManager *manager, CommandRegistry *registry, QWidget *parent)
    : QWidget(parent)
    , m_manager(manager)
    , m_registry(registry)
{
    setObjectName(QStringLiteral("keybindingEditor"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({tr("Command"), tr("Shortcut")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    layout->addWidget(m_table);

    auto *row = new QHBoxLayout;
    m_sequenceEdit = new QKeySequenceEdit(this);
    auto *assign = new QPushButton(tr("Assign"), this);
    auto *reset = new QPushButton(tr("Reset"), this);
    row->addWidget(m_sequenceEdit, 1);
    row->addWidget(assign);
    row->addWidget(reset);
    layout->addLayout(row);

    m_status = new QLabel(this);
    layout->addWidget(m_status);

    connect(assign, &QPushButton::clicked, this, &KeybindingEditor::assignCurrent);
    connect(reset, &QPushButton::clicked, this, &KeybindingEditor::resetCurrent);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this]() {
        const QString id = selectedCommandId();
        if (id.isEmpty() || !m_manager) {
            return;
        }
        m_sequenceEdit->setKeySequence(m_manager->shortcut(id));
        m_status->clear();
    });

    populate();
}

void KeybindingEditor::reload()
{
    populate();
}

void KeybindingEditor::populate()
{
    if (!m_registry || !m_manager) {
        return;
    }

    const QString previous = selectedCommandId();
    m_table->setRowCount(0);
    const QStringList ids = m_registry->ids();
    m_table->setRowCount(ids.size());
    for (int row = 0; row < ids.size(); ++row) {
        const QString id = ids.at(row);
        QString label = id;
        if (QAction *action = m_registry->action(id)) {
            label = action->text().remove(QLatin1Char('&'));
        }
        auto *commandItem = new QTableWidgetItem(label);
        commandItem->setData(Qt::UserRole, id);
        m_table->setItem(row, 0, commandItem);
        m_table->setItem(row, 1, new QTableWidgetItem(m_manager->shortcut(id).toString(QKeySequence::NativeText)));
        if (id == previous) {
            m_table->selectRow(row);
        }
    }
    if (m_table->selectedItems().isEmpty() && m_table->rowCount() > 0) {
        m_table->selectRow(0);
    }
}

QString KeybindingEditor::selectedCommandId() const
{
    const int row = m_table->currentRow();
    if (row < 0) {
        return {};
    }
    QTableWidgetItem *item = m_table->item(row, 0);
    return item ? item->data(Qt::UserRole).toString() : QString();
}

void KeybindingEditor::assignCurrent()
{
    const QString id = selectedCommandId();
    if (id.isEmpty() || !m_manager) {
        return;
    }
    QString conflict;
    if (!m_manager->setShortcut(id, m_sequenceEdit->keySequence(), &conflict)) {
        m_status->setText(tr("Conflict with %1").arg(conflict));
        return;
    }
    m_status->setText(tr("Assigned"));
    populate();
}

void KeybindingEditor::resetCurrent()
{
    const QString id = selectedCommandId();
    if (id.isEmpty() || !m_manager) {
        return;
    }
    m_manager->resetShortcut(id);
    m_sequenceEdit->setKeySequence(m_manager->shortcut(id));
    m_status->setText(tr("Reset to default"));
    populate();
}
