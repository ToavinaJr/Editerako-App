#include "ui/FuzzyPickerDialog.h"

#include "core/FuzzyMatcher.h"

#include <QAbstractItemView>
#include <QEvent>
#include <QKeyEvent>
#include <QtGlobal>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

namespace {

constexpr int kIdRole = Qt::UserRole;
constexpr int kEnabledRole = Qt::UserRole + 1;

} // namespace

FuzzyPickerDialog::FuzzyPickerDialog(QWidget *parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("fuzzyPickerDialog"));
    setModal(true);
    setWindowModality(Qt::WindowModal);
    resize(640, 420);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    m_edit = new QLineEdit(this);
    m_edit->setClearButtonEnabled(true);
    m_edit->installEventFilter(this);
    layout->addWidget(m_edit);

    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("fuzzyPickerList"));
    m_list->setUniformItemSizes(true);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_list, 1);

    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("fuzzyPickerStatus"));
    layout->addWidget(m_status);

    connect(m_edit, &QLineEdit::textChanged, this, &FuzzyPickerDialog::refreshList);
    connect(m_list, &QListWidget::itemActivated, this, [this](QListWidgetItem *) {
        acceptCurrent();
    });

    m_edit->setFocus();
}

void FuzzyPickerDialog::setPlaceholderText(const QString &text)
{
    m_edit->setPlaceholderText(text);
}

void FuzzyPickerDialog::setStatusText(const QString &text)
{
    m_status->setText(text);
}

void FuzzyPickerDialog::setItems(const QList<FuzzyPickerItem> &items)
{
    m_items = items;
    refreshList();
}

QString FuzzyPickerDialog::selectedId() const
{
    const QListWidgetItem *item = m_list->currentItem();
    return item ? item->data(kIdRole).toString() : QString();
}

QString FuzzyPickerDialog::query() const
{
    return m_edit->text();
}

QString FuzzyPickerDialog::rankQuery() const
{
    return query();
}

bool FuzzyPickerDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_edit && event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Down) {
            moveSelection(1);
            return true;
        }
        if (key->key() == Qt::Key_Up) {
            moveSelection(-1);
            return true;
        }
        if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
            acceptCurrent();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void FuzzyPickerDialog::refreshList()
{
    const QString needle = rankQuery();
    QStringList candidates;
    candidates.reserve(m_items.size());
    for (const FuzzyPickerItem &item : m_items) {
        candidates.append(item.filterText.isEmpty() ? item.display : item.filterText);
    }

    const QList<FuzzyMatch> matches = fuzzyRank(candidates, needle);
    m_list->clear();
    for (const FuzzyMatch &match : matches) {
        const FuzzyPickerItem &item = m_items.at(match.index);
        QString text = item.display;
        if (!item.hint.isEmpty()) {
            text += QLatin1String("    ") + item.hint;
        }
        auto *row = new QListWidgetItem(text, m_list);
        row->setData(kIdRole, item.id);
        row->setData(kEnabledRole, item.enabled);
        if (!item.enabled) {
            row->setForeground(palette().placeholderText());
        }
    }

    if (m_list->count() > 0) {
        m_list->setCurrentRow(0);
    }
}

void FuzzyPickerDialog::moveSelection(int delta)
{
    if (m_list->count() == 0) {
        return;
    }
    const int next = qBound(0, m_list->currentRow() + delta, m_list->count() - 1);
    m_list->setCurrentRow(next);
}

void FuzzyPickerDialog::acceptCurrent()
{
    QListWidgetItem *item = m_list->currentItem();
    if (!item || !item->data(kEnabledRole).toBool()) {
        return;
    }
    accept();
}
