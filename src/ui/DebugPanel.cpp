#include "ui/DebugPanel.h"

#include "app/DebugSession.h"
#include "debug/LaunchFile.h"

#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTextCursor>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

QTreeWidgetItem *findItemByRef(QTreeWidget *tree, QTreeWidgetItem *parent, int ref)
{
    const int count = parent ? parent->childCount() : tree->topLevelItemCount();
    for (int i = 0; i < count; ++i) {
        QTreeWidgetItem *item = parent ? parent->child(i) : tree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toInt() == ref) {
            return item;
        }
        if (auto *found = findItemByRef(tree, item, ref)) {
            return found;
        }
    }
    return nullptr;
}

} // namespace

DebugPanel::DebugPanel(DebugSession *session, QWidget *parent)
    : QWidget(parent)
    , m_session(session)
    , m_configs(new QComboBox(this))
    , m_start(new QPushButton(tr("Start"), this))
    , m_stop(new QPushButton(tr("Stop"), this))
    , m_continue(new QPushButton(tr("Continue"), this))
    , m_pause(new QPushButton(tr("Pause"), this))
    , m_stepOver(new QPushButton(tr("Step Over"), this))
    , m_stepInto(new QPushButton(tr("Step Into"), this))
    , m_stepOut(new QPushButton(tr("Step Out"), this))
    , m_createLaunch(new QPushButton(tr("Create launch.json"), this))
    , m_stack(new QTreeWidget(this))
    , m_variables(new QTreeWidget(this))
    , m_console(new QPlainTextEdit(this))
    , m_repl(new QLineEdit(this))
{
    setObjectName(QStringLiteral("debugPanel"));

    m_start->setObjectName(QStringLiteral("debugStart"));
    m_stop->setObjectName(QStringLiteral("debugStop"));
    m_configs->setObjectName(QStringLiteral("debugConfigurations"));
    m_stack->setObjectName(QStringLiteral("debugStack"));
    m_variables->setObjectName(QStringLiteral("debugVariables"));
    m_console->setObjectName(QStringLiteral("debugConsole"));
    m_repl->setObjectName(QStringLiteral("debugRepl"));

    m_stack->setHeaderLabels({tr("Call Stack")});
    m_stack->setRootIsDecorated(false);
    m_stack->header()->setStretchLastSection(true);
    m_variables->setHeaderLabels({tr("Name"), tr("Value"), tr("Type")});
    m_variables->header()->setStretchLastSection(true);
    m_console->setReadOnly(true);
    m_console->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_console->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_repl->setPlaceholderText(tr("Evaluate expression…"));

    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(0, 0, 0, 0);
    toolbar->addWidget(m_start);
    toolbar->addWidget(m_stop);
    toolbar->addWidget(m_continue);
    toolbar->addWidget(m_pause);
    toolbar->addWidget(m_stepOver);
    toolbar->addWidget(m_stepInto);
    toolbar->addWidget(m_stepOut);
    toolbar->addWidget(m_configs, 1);
    toolbar->addWidget(m_createLaunch);

    auto *trees = new QSplitter(Qt::Horizontal, this);
    trees->addWidget(m_stack);
    trees->addWidget(m_variables);
    trees->setStretchFactor(0, 1);
    trees->setStretchFactor(1, 2);

    auto *consoleBox = new QWidget(this);
    auto *consoleLayout = new QVBoxLayout(consoleBox);
    consoleLayout->setContentsMargins(0, 0, 0, 0);
    consoleLayout->setSpacing(4);
    auto *consoleLabel = new QLabel(tr("Debug Console"), consoleBox);
    consoleLayout->addWidget(consoleLabel);
    consoleLayout->addWidget(m_console, 1);
    consoleLayout->addWidget(m_repl);

    auto *split = new QSplitter(Qt::Vertical, this);
    split->addWidget(trees);
    split->addWidget(consoleBox);
    split->setStretchFactor(0, 2);
    split->setStretchFactor(1, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);
    layout->addLayout(toolbar);
    layout->addWidget(split, 1);

    if (!m_session) {
        setEnabled(false);
        return;
    }

    connect(m_start, &QPushButton::clicked, m_session, &DebugSession::start);
    connect(m_stop, &QPushButton::clicked, m_session, &DebugSession::stop);
    connect(m_continue, &QPushButton::clicked, m_session, &DebugSession::continueRun);
    connect(m_pause, &QPushButton::clicked, m_session, &DebugSession::pause);
    connect(m_stepOver, &QPushButton::clicked, m_session, &DebugSession::stepOver);
    connect(m_stepInto, &QPushButton::clicked, m_session, &DebugSession::stepInto);
    connect(m_stepOut, &QPushButton::clicked, m_session, &DebugSession::stepOut);
    connect(m_createLaunch, &QPushButton::clicked, this, [this]() {
        QString error;
        if (!m_session->createLaunchFile(&error)) {
            QMessageBox::warning(this, tr("Debug"),
                                 error.isEmpty() ? tr("Could not write launch.json") : error);
        }
    });
    connect(m_configs, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_session->setSelectedConfiguration(m_configs->itemText(index));
    });
    connect(m_stack, &QTreeWidget::itemActivated, this, [this]() { onStackActivated(); });
    connect(m_stack, &QTreeWidget::itemClicked, this, [this]() { onStackActivated(); });
    connect(m_variables, &QTreeWidget::itemExpanded, this, &DebugPanel::onVariableExpanded);
    connect(m_repl, &QLineEdit::returnPressed, this, &DebugPanel::submitEvaluate);

    connect(m_session, &DebugSession::configurationsChanged, this, &DebugPanel::rebuildConfigurations);
    connect(m_session, &DebugSession::stateChanged, this, [this](DebugSession::State) { updateButtons(); });
    connect(m_session, &DebugSession::stackFramesChanged, this, &DebugPanel::setStackFrames);
    connect(m_session, &DebugSession::scopesChanged, this, &DebugPanel::setScopes);
    connect(m_session, &DebugSession::variablesReady, this, &DebugPanel::addVariables);
    connect(m_session, &DebugSession::outputReceived, this, &DebugPanel::appendOutput);

    rebuildConfigurations();
    updateButtons();
}

void DebugPanel::rebuildConfigurations()
{
    const QString current = m_session ? m_session->selectedConfiguration() : QString();
    m_configs->blockSignals(true);
    m_configs->clear();
    if (m_session) {
        for (const LaunchConfiguration &config : m_session->configurations()) {
            m_configs->addItem(config.name);
        }
        if (!current.isEmpty()) {
            const int index = m_configs->findText(current);
            if (index >= 0) {
                m_configs->setCurrentIndex(index);
            }
        }
        if (m_configs->count() > 0) {
            m_session->setSelectedConfiguration(m_configs->currentText());
        }
    }
    m_configs->blockSignals(false);
    m_configs->setEnabled(m_configs->count() > 0);
}

void DebugPanel::updateButtons()
{
    const auto state = m_session ? m_session->state() : DebugSession::State::Idle;
    const bool starting = state == DebugSession::State::Starting;
    const bool running = state == DebugSession::State::Running;
    const bool stopped = state == DebugSession::State::Stopped;
    const bool active = starting || running || stopped;
    m_start->setEnabled(!starting && !running);
    m_start->setText(stopped ? tr("Continue") : tr("Start"));
    m_stop->setEnabled(active);
    m_continue->setEnabled(stopped);
    m_pause->setEnabled(running);
    m_stepOver->setEnabled(stopped);
    m_stepInto->setEnabled(stopped);
    m_stepOut->setEnabled(stopped);
}

void DebugPanel::setStackFrames(const QVector<DapStackFrame> &frames)
{
    m_stack->clear();
    for (const DapStackFrame &frame : frames) {
        const QString location = frame.sourcePath.isEmpty()
            ? frame.name
            : QStringLiteral("%1  %2:%3").arg(frame.name, frame.sourcePath).arg(frame.line);
        auto *item = new QTreeWidgetItem(m_stack, {location});
        item->setData(0, Qt::UserRole, frame.id);
    }
}

void DebugPanel::setScopes(const QVector<DapScope> &scopes)
{
    m_variables->clear();
    for (const DapScope &scope : scopes) {
        auto *item = new QTreeWidgetItem(m_variables, {scope.name, QString(), QString()});
        item->setData(0, Qt::UserRole, scope.variablesReference);
        if (scope.variablesReference > 0) {
            item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        }
    }
}

void DebugPanel::addVariables(int variablesReference, const QVector<DapVariable> &variables)
{
    QTreeWidgetItem *parent = findItemByRef(m_variables, nullptr, variablesReference);
    if (!parent) {
        return;
    }
    parent->takeChildren();
    for (const DapVariable &variable : variables) {
        auto *item = new QTreeWidgetItem(parent, {variable.name, variable.value, variable.type});
        item->setData(0, Qt::UserRole, variable.variablesReference);
        if (variable.variablesReference > 0) {
            item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        }
    }
    parent->setExpanded(true);
}

void DebugPanel::onVariableExpanded(QTreeWidgetItem *item)
{
    if (!item || !m_session || item->childCount() > 0) {
        return;
    }
    const int ref = item->data(0, Qt::UserRole).toInt();
    if (ref > 0) {
        m_session->requestVariables(ref);
    }
}

void DebugPanel::onStackActivated()
{
    QTreeWidgetItem *item = m_stack->currentItem();
    if (!item || !m_session) {
        return;
    }
    m_session->selectStackFrame(item->data(0, Qt::UserRole).toInt());
}

void DebugPanel::appendOutput(const QString &category, const QString &text)
{
    Q_UNUSED(category);
    m_console->moveCursor(QTextCursor::End);
    m_console->insertPlainText(text);
    m_console->moveCursor(QTextCursor::End);
}

void DebugPanel::submitEvaluate()
{
    if (!m_session) {
        return;
    }
    const QString text = m_repl->text();
    m_repl->clear();
    m_session->evaluate(text);
}
