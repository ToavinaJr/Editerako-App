#include "ui/TasksPanel.h"

#include "tasks/TaskManager.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

constexpr int kTaskIdRole = Qt::UserRole;

} // namespace

TasksPanel::TasksPanel(TaskManager *manager, QWidget *parent)
    : QWidget(parent)
    , m_manager(manager)
    , m_header(new QLabel(this))
    , m_preset(new QComboBox(this))
    , m_target(new QLineEdit(this))
    , m_list(new QListWidget(this))
    , m_run(new QPushButton(tr("Run Task"), this))
    , m_cancel(new QPushButton(tr("Cancel"), this))
    , m_configure(new QPushButton(tr("Configure"), this))
    , m_build(new QPushButton(tr("Build"), this))
    , m_clean(new QPushButton(tr("Clean"), this))
    , m_test(new QPushButton(tr("Test"), this))
    , m_launch(new QPushButton(tr("Run"), this))
{
    setObjectName(QStringLiteral("tasksPanel"));
    m_list->setObjectName(QStringLiteral("tasksList"));
    m_target->setPlaceholderText(tr("Target (optional)"));

    auto *cmakeRow = new QHBoxLayout;
    cmakeRow->addWidget(new QLabel(tr("Preset"), this));
    cmakeRow->addWidget(m_preset, 1);
    cmakeRow->addWidget(m_target, 1);

    auto *cmakeButtons = new QHBoxLayout;
    cmakeButtons->addWidget(m_configure);
    cmakeButtons->addWidget(m_build);
    cmakeButtons->addWidget(m_clean);
    cmakeButtons->addWidget(m_test);
    cmakeButtons->addWidget(m_launch);
    cmakeButtons->addStretch();

    auto *taskButtons = new QHBoxLayout;
    taskButtons->addWidget(m_run);
    taskButtons->addWidget(m_cancel);
    taskButtons->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addWidget(m_header);
    layout->addLayout(cmakeRow);
    layout->addLayout(cmakeButtons);
    layout->addWidget(m_list, 1);
    layout->addLayout(taskButtons);

    connect(m_manager, &TaskManager::tasksChanged, this, &TasksPanel::rebuild);
    connect(m_manager, &TaskManager::cmakeChanged, this, &TasksPanel::rebuild);
    connect(m_manager, &TaskManager::started, this, &TasksPanel::updateRunning);
    connect(m_manager, &TaskManager::finished, this, [this](int) { updateRunning(); });
    connect(m_manager, &TaskManager::failed, this, [this](const QString &) { updateRunning(); });
    connect(m_preset, &QComboBox::currentTextChanged, m_manager, &TaskManager::setPreset);
    connect(m_target, &QLineEdit::textChanged, m_manager, &TaskManager::setTarget);
    connect(m_run, &QPushButton::clicked, this, &TasksPanel::runSelected);
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) {
        runSelected();
    });
    connect(m_cancel, &QPushButton::clicked, m_manager, &TaskManager::cancel);
    connect(m_configure, &QPushButton::clicked, this, [this]() {
        m_manager->run(QStringLiteral("cmake.configure"));
    });
    connect(m_build, &QPushButton::clicked, this, [this]() {
        m_manager->run(QStringLiteral("cmake.build"));
    });
    connect(m_clean, &QPushButton::clicked, this, [this]() {
        m_manager->run(QStringLiteral("cmake.clean"));
    });
    connect(m_test, &QPushButton::clicked, this, [this]() {
        m_manager->run(QStringLiteral("cmake.test"));
    });
    connect(m_launch, &QPushButton::clicked, this, [this]() {
        m_manager->run(QStringLiteral("cmake.run"));
    });

    rebuild();
}

void TasksPanel::setWorkspace(const QString &path)
{
    m_manager->setWorkspace(path);
}

void TasksPanel::runSelected()
{
    const QListWidgetItem *item = m_list->currentItem();
    if (!item) {
        m_manager->runBuild();
        return;
    }
    m_manager->run(item->data(kTaskIdRole).toString());
}

void TasksPanel::rebuild()
{
    const CMakeWorkspace cmake = m_manager->cmake();
    m_header->setText(cmake.detected ? tr("CMake project") : tr("No CMakeLists.txt"));
    const bool cmakeReady = cmake.detected && !cmake.cmakeExecutable.isEmpty();
    m_preset->setEnabled(cmakeReady);
    m_target->setEnabled(cmakeReady);
    m_configure->setEnabled(cmakeReady);
    m_build->setEnabled(cmakeReady);
    m_clean->setEnabled(cmakeReady);
    m_test->setEnabled(cmakeReady);
    m_launch->setEnabled(cmakeReady);

    const QString previous = m_preset->currentText();
    const QSignalBlocker blocker(m_preset);
    m_preset->clear();
    m_preset->addItems(cmake.visiblePresetNames());
    const int idx = m_preset->findText(m_manager->selectedPreset());
    if (idx >= 0) {
        m_preset->setCurrentIndex(idx);
    } else if (!previous.isEmpty()) {
        const int fallback = m_preset->findText(previous);
        if (fallback >= 0) {
            m_preset->setCurrentIndex(fallback);
        }
    }

    const QString selectedId = m_list->currentItem()
        ? m_list->currentItem()->data(kTaskIdRole).toString()
        : QString();
    m_list->clear();
    int restore = -1;
    const QVector<TaskDefinition> tasks = m_manager->tasks();
    for (int i = 0; i < tasks.size(); ++i) {
        auto *item = new QListWidgetItem(tasks.at(i).label, m_list);
        item->setData(kTaskIdRole, tasks.at(i).id);
        if (tasks.at(i).id == selectedId) {
            restore = i;
        }
    }
    if (restore >= 0) {
        m_list->setCurrentRow(restore);
    } else if (m_list->count() > 0) {
        m_list->setCurrentRow(0);
    }
    updateRunning();
}

void TasksPanel::updateRunning()
{
    const bool running = m_manager->isRunning();
    m_run->setEnabled(!running && m_list->count() > 0);
    m_cancel->setEnabled(running);
}
