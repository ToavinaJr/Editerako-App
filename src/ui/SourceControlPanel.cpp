#include "ui/SourceControlPanel.h"

#include "scm/GitCliProvider.h"
#include "scm/GitParsers.h"

#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace {
constexpr int kPathRole = Qt::UserRole;
constexpr int kStagedRole = Qt::UserRole + 1;
constexpr int kStateRole = Qt::UserRole + 2;

QString stateText(ScmFileState state)
{
    switch (state) {
    case ScmFileState::Modified: return QStringLiteral("M");
    case ScmFileState::Added: return QStringLiteral("A");
    case ScmFileState::Deleted: return QStringLiteral("D");
    case ScmFileState::Renamed: return QStringLiteral("R");
    case ScmFileState::Copied: return QStringLiteral("C");
    case ScmFileState::Untracked: return QStringLiteral("U");
    case ScmFileState::Conflicted: return QStringLiteral("!");
    default: return QStringLiteral("?");
    }
}
}

SourceControlPanel::SourceControlPanel(GitCliProvider *provider, QWidget *parent)
    : QWidget(parent), m_provider(provider), m_header(new QLabel(tr("No repository"), this)),
      m_message(new QLineEdit(this)), m_tree(new QTreeWidget(this)),
      m_stage(new QPushButton(tr("Stage"), this)), m_unstage(new QPushButton(tr("Unstage"), this)),
      m_discard(new QPushButton(tr("Discard"), this))
{
    setObjectName(QStringLiteral("sourceControlPanel"));
    m_tree->setObjectName(QStringLiteral("sourceControlTree"));
    m_tree->setHeaderLabels({tr("File"), tr("State")});
    m_message->setPlaceholderText(tr("Commit message"));
    auto *commit = new QPushButton(tr("Commit"), this);
    auto *refresh = new QPushButton(tr("Refresh"), this);
    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_stage); buttons->addWidget(m_unstage); buttons->addWidget(m_discard); buttons->addStretch(); buttons->addWidget(refresh);
    auto *commitRow = new QHBoxLayout;
    commitRow->addWidget(m_message); commitRow->addWidget(commit);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_header); layout->addLayout(commitRow); layout->addWidget(m_tree); layout->addLayout(buttons);

    connect(provider, &GitCliProvider::statusChanged, this, &SourceControlPanel::rebuild);
    connect(provider, &GitCliProvider::operationFailed, this, [this](const ScmError &error) { QMessageBox::warning(this, tr("Source Control"), error.message); });
    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, &SourceControlPanel::updateActions);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *, int) { if (!selectedPath().isEmpty()) emit diffRequested(selectedPath(), selectedStaged()); });
    connect(m_stage, &QPushButton::clicked, this, [this]() { m_provider->stage({selectedPath()}); });
    connect(m_unstage, &QPushButton::clicked, this, [this]() { m_provider->unstage({selectedPath()}); });
    connect(m_discard, &QPushButton::clicked, this, [this]() {
        const QString path = selectedPath();
        if (!path.isEmpty() && QMessageBox::question(this, tr("Discard Changes"), tr("Discard all working-tree changes to %1?").arg(path)) == QMessageBox::Yes) {
            m_provider->discard({path});
        }
    });
    connect(refresh, &QPushButton::clicked, provider, &GitCliProvider::refresh);
    connect(commit, &QPushButton::clicked, this, [this]() { const QString text = m_message->text().trimmed(); if (!text.isEmpty()) { m_provider->commit(text); m_message->clear(); } });
    updateActions();
}

void SourceControlPanel::setWorkspace(const QString &path) { m_provider->setWorkspace(path); }

void SourceControlPanel::rebuild(const ScmStatus &status)
{
    m_tree->clear();
    if (!status.isRepository) {
        m_header->setText(tr("No Git repository"));
    } else {
        QString name = status.branch.isEmpty() ? tr("detached") : status.branch;
        const QString tracking = GitParsers::aheadBehindLabel(status);
        if (!tracking.isEmpty()) {
            name += QLatin1Char(' ') + tracking;
        }
        m_header->setText(tr("Branch: %1").arg(name));
    }
    if (!status.isRepository) { updateActions(); return; }
    auto *staged = new QTreeWidgetItem(m_tree, {tr("Staged Changes")});
    auto *changes = new QTreeWidgetItem(m_tree, {tr("Changes")});
    auto *untracked = new QTreeWidgetItem(m_tree, {tr("Untracked")});
    const QDir root(status.repositoryRoot);
    for (const ScmChange &change : status.changes) {
        QTreeWidgetItem *parent = change.state == ScmFileState::Untracked ? untracked : (change.staged ? staged : changes);
        QString label = change.path;
        if (!status.repositoryRoot.isEmpty()) {
            const QString relative = QDir::fromNativeSeparators(root.relativeFilePath(change.path));
            if (!relative.startsWith(QLatin1String(".."))) {
                label = relative;
            }
        }
        auto *item = new QTreeWidgetItem(parent, {label, stateText(change.state)});
        item->setData(0, kPathRole, change.path);
        item->setData(0, kStagedRole, change.staged);
        item->setData(0, kStateRole, static_cast<int>(change.state));
        item->setToolTip(0, change.path);
        if (!change.oldPath.isEmpty()) {
            item->setToolTip(0, tr("Renamed from %1").arg(change.oldPath));
        }
    }
    for (QTreeWidgetItem *group : {staged, changes, untracked}) {
        group->setText(0, QStringLiteral("%1 (%2)").arg(group->text(0)).arg(group->childCount()));
        group->setExpanded(true);
    }
    updateActions();
}

QString SourceControlPanel::selectedPath() const { auto *item = m_tree->currentItem(); return item ? item->data(0, kPathRole).toString() : QString(); }
bool SourceControlPanel::selectedStaged() const { auto *item = m_tree->currentItem(); return item && item->data(0, kStagedRole).toBool(); }
void SourceControlPanel::updateActions()
{
    const bool has = !selectedPath().isEmpty();
    const bool staged = selectedStaged();
    m_stage->setEnabled(has && !staged);
    m_unstage->setEnabled(has && staged);
    m_discard->setEnabled(has && !staged);
}
