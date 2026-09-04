#include "viewers/CsvViewer.h"

#include "viewers/CsvParser.h"

#include <QAbstractItemView>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>
#include <QtGlobal>

namespace {

QString readUtf8File(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QByteArray bytes = file.readAll();
    if (bytes.startsWith("\xEF\xBB\xBF")) {
        bytes.remove(0, 3);
    }
    return QString::fromUtf8(bytes);
}

int columnCountOf(const QVector<QStringList> &rows)
{
    int columns = 0;
    for (const QStringList &row : rows) {
        columns = qMax(columns, static_cast<int>(row.size()));
    }
    return columns;
}

} // namespace

CsvViewer::CsvViewer(QWidget *parent)
    : QWidget(parent)
    , m_table(new QTableView(this))
    , m_model(new QStandardItemModel(this))
{
    m_table->setObjectName(QStringLiteral("csvTable"));
    m_table->setModel(m_model);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_table);
}

bool CsvViewer::load(const QString &filePath)
{
    const QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        return false;
    }

    const QVector<QStringList> rows = parseCsv(readUtf8File(filePath));
    m_model->clear();

    const int columns = columnCountOf(rows);
    if (rows.isEmpty() || columns == 0) {
        m_filePath = filePath;
        return true;
    }

    const QStringList &header = rows.front();
    QStringList labels;
    labels.reserve(columns);
    for (int c = 0; c < columns; ++c) {
        labels.append(c < header.size() ? header.at(c)
                                        : QStringLiteral("Column %1").arg(c + 1));
    }
    m_model->setHorizontalHeaderLabels(labels);

    for (int r = 1; r < rows.size(); ++r) {
        QList<QStandardItem *> items;
        items.reserve(columns);
        for (int c = 0; c < columns; ++c) {
            const QString value = c < rows.at(r).size() ? rows.at(r).at(c) : QString();
            auto *item = new QStandardItem(value);
            item->setEditable(false);
            items.append(item);
        }
        m_model->appendRow(items);
    }

    m_filePath = filePath;
    return true;
}
