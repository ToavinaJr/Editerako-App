#include "ui/ProblemsPanel.h"

#include "editor/DiagnosticMarkup.h"

#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QAbstractItemView>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace {

constexpr int kPathRole = Qt::UserRole;
constexpr int kLineRole = Qt::UserRole + 1;
constexpr int kColumnRole = Qt::UserRole + 2;

QString severityLabel(EditorDiagnostic::Severity severity)
{
    switch (severity) {
    case EditorDiagnostic::Severity::Error:
        return QObject::tr("Error");
    case EditorDiagnostic::Severity::Warning:
        return QObject::tr("Warning");
    case EditorDiagnostic::Severity::Information:
        return QObject::tr("Info");
    case EditorDiagnostic::Severity::Hint:
        return QObject::tr("Hint");
    }
    return QObject::tr("Error");
}

} // namespace

ProblemsPanel::ProblemsPanel(QWidget *parent)
    : QWidget(parent)
    , m_model(new ProblemModel(this))
    , m_filter(new QComboBox(this))
    , m_summary(new QLabel(this))
    , m_tree(new QTreeWidget(this))
{
    setObjectName(QStringLiteral("problemsPanel"));

    m_filter->setObjectName(QStringLiteral("problemsFilter"));
    m_filter->addItem(tr("Problems"), static_cast<int>(ProblemModel::Filter::All));
    m_filter->addItem(tr("Errors"), static_cast<int>(ProblemModel::Filter::Errors));
    m_filter->addItem(tr("Warnings"), static_cast<int>(ProblemModel::Filter::Warnings));

    m_summary->setObjectName(QStringLiteral("problemsSummary"));

    m_tree->setObjectName(QStringLiteral("problemsTree"));
    m_tree->setHeaderHidden(true);
    m_tree->setRootIsDecorated(true);
    m_tree->setUniformRowHeights(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->header()->setStretchLastSection(true);

    auto *toolbar = new QHBoxLayout;
    toolbar->addWidget(m_filter);
    toolbar->addWidget(m_summary, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addLayout(toolbar);
    layout->addWidget(m_tree, 1);

    connect(m_filter, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_model->setFilter(static_cast<ProblemModel::Filter>(m_filter->itemData(index).toInt()));
    });
    connect(m_model, &ProblemModel::changed, this, &ProblemsPanel::rebuild);
    connect(m_tree, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *item, int) {
        activateItem(item);
    });
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
        activateItem(item);
    });

    rebuild();
}

void ProblemsPanel::setWorkspaceRoot(const QString &root)
{
    if (m_workspaceRoot == root) {
        return;
    }
    m_workspaceRoot = root;
    rebuild();
}

QString ProblemsPanel::displayPath(const QString &path) const
{
    if (m_workspaceRoot.isEmpty()) {
        return QFileInfo(path).fileName();
    }
    const QString root = QFileInfo(m_workspaceRoot).absoluteFilePath();
    const QString abs = QFileInfo(path).absoluteFilePath();
    if (abs.startsWith(root, Qt::CaseInsensitive)) {
        QString rel = abs.mid(root.size());
        if (rel.startsWith(QLatin1Char('/')) || rel.startsWith(QLatin1Char('\\'))) {
            rel.remove(0, 1);
        }
        if (!rel.isEmpty()) {
            return QDir::fromNativeSeparators(rel);
        }
    }
    return QFileInfo(path).fileName();
}

void ProblemsPanel::rebuild()
{
    m_tree->clear();
    const QVector<ProblemItem> items = m_model->visibleItems();
    QHash<QString, QTreeWidgetItem *> files;
    for (const ProblemItem &item : items) {
        QTreeWidgetItem *parent = files.value(item.path);
        if (!parent) {
            parent = new QTreeWidgetItem(m_tree);
            parent->setText(0, displayPath(item.path));
            parent->setToolTip(0, item.path);
            parent->setData(0, kPathRole, item.path);
            parent->setExpanded(true);
            files.insert(item.path, parent);
        }
        auto *row = new QTreeWidgetItem(parent);
        const QString text = tr("%1:%2  %3  %4")
                                 .arg(item.line + 1)
                                 .arg(item.column + 1)
                                 .arg(severityLabel(item.severity), item.message);
        row->setText(0, text);
        row->setForeground(0, diagnosticColor(item.severity));
        row->setData(0, kPathRole, item.path);
        row->setData(0, kLineRole, item.line);
        row->setData(0, kColumnRole, item.column);
        row->setToolTip(0, item.path);
    }

    const int errors = m_model->errorCount();
    const int warnings = m_model->warningCount();
    m_summary->setText(tr("%1 errors, %2 warnings").arg(errors).arg(warnings));
}

void ProblemsPanel::activateItem(QTreeWidgetItem *item)
{
    if (!item) {
        return;
    }
    const QString path = item->data(0, kPathRole).toString();
    if (path.isEmpty()) {
        return;
    }
    if (!item->data(0, kLineRole).isValid() && item->childCount() > 0) {
        activateItem(item->child(0));
        return;
    }
    emit problemActivated(path, item->data(0, kLineRole).toInt(),
                          item->data(0, kColumnRole).toInt());
}
