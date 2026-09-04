#include "ui/WelcomeDialog.h"

#include <QAction>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

WelcomeDialog::WelcomeDialog(const QStringList &recents, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Welcome to Editerako"));
    setObjectName(QStringLiteral("welcomeDialog"));
    setModal(true);
    resize(520, 380);

    auto *root = new QVBoxLayout(this);
    root->addWidget(new QLabel(tr("What would you like to open?"), this));

    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("welcomeRecentList"));
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    for (const QString &path : recents) {
        auto *item = new QListWidgetItem(QDir::toNativeSeparators(path), m_list);
        item->setData(Qt::UserRole, path);
        item->setToolTip(QDir::toNativeSeparators(path));
    }
    if (recents.isEmpty()) {
        auto *empty = new QListWidgetItem(tr("No recent workspaces"), m_list);
        empty->setFlags(empty->flags() & ~Qt::ItemIsEnabled);
    } else {
        m_list->setCurrentRow(0);
    }
    root->addWidget(m_list, 1);

    auto *buttons = new QHBoxLayout;
    m_openRecent = new QPushButton(tr("Open Selected"), this);
    m_openRecent->setObjectName(QStringLiteral("welcomeOpenRecent"));
    m_openRecent->setEnabled(false);
    auto *folder = new QPushButton(tr("Open Folder"), this);
    folder->setObjectName(QStringLiteral("welcomeOpenFolder"));
    auto *file = new QPushButton(tr("Open File"), this);
    file->setObjectName(QStringLiteral("welcomeOpenFile"));
    auto *cancel = new QPushButton(tr("Cancel"), this);
    cancel->setObjectName(QStringLiteral("welcomeCancel"));
    buttons->addWidget(m_openRecent);
    buttons->addStretch(1);
    buttons->addWidget(folder);
    buttons->addWidget(file);
    buttons->addWidget(cancel);
    root->addLayout(buttons);

    connect(m_list, &QListWidget::itemSelectionChanged, this, [this]() {
        QListWidgetItem *item = m_list->currentItem();
        m_openRecent->setEnabled(item && item->flags().testFlag(Qt::ItemIsEnabled) &&
                                 item->data(Qt::UserRole).isValid());
    });
    connect(
        m_list, &QListWidget::itemActivated, this, [this](QListWidgetItem *) { acceptRecent(); });
    connect(m_list, &QListWidget::customContextMenuRequested, this, &WelcomeDialog::showListMenu);
    connect(m_openRecent, &QPushButton::clicked, this, &WelcomeDialog::acceptRecent);
    connect(folder, &QPushButton::clicked, this, [this]() {
        m_choice = OpenFolder;
        accept();
    });
    connect(file, &QPushButton::clicked, this, [this]() {
        m_choice = OpenFile;
        accept();
    });
    connect(cancel, &QPushButton::clicked, this, [this]() {
        m_choice = Canceled;
        reject();
    });
}

void WelcomeDialog::acceptRecent()
{
    QListWidgetItem *item = m_list->currentItem();
    if (!item || !item->data(Qt::UserRole).isValid()) {
        return;
    }
    m_selected = item->data(Qt::UserRole).toString();
    m_choice = OpenRecent;
    accept();
}

void WelcomeDialog::showListMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_list->itemAt(pos);
    if (!item || !item->data(Qt::UserRole).isValid()) {
        return;
    }
    QMenu menu(this);
    QAction *remove = menu.addAction(tr("Remove from List"));
    if (menu.exec(m_list->mapToGlobal(pos)) != remove) {
        return;
    }
    const QString path = item->data(Qt::UserRole).toString();
    delete item;
    emit removeRequested(path);
    if (m_list->count() == 0) {
        auto *empty = new QListWidgetItem(tr("No recent workspaces"), m_list);
        empty->setFlags(empty->flags() & ~Qt::ItemIsEnabled);
    }
    m_openRecent->setEnabled(false);
}
