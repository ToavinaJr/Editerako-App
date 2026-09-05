#include "ui/SettingsDialog.h"

#include "ai/AiCatalog.h"
#include "core/AppSettings.h"
#include "core/ThemeManager.h"
#include "plugins/PluginManager.h"
#include "terminal/PtyTerminalBackend.h"
#include "terminal/ShellProfiles.h"
#include "ui/KeybindingEditor.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFont>
#include <QFontComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <QtGlobal>

namespace {

QWidget *wrapForm(QFormLayout *form)
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->addLayout(form);
    layout->addStretch(1);
    return page;
}

qint64 mbToBytes(int mb)
{
    return static_cast<qint64>(mb) * 1024 * 1024;
}

int bytesToMb(qint64 bytes)
{
    return qMax(1, static_cast<int>((bytes + (1024 * 1024 / 2)) / (1024 * 1024)));
}

} // namespace

SettingsDialog::SettingsDialog(KeybindingManager *keybindings,
                               CommandRegistry *commands,
                               PluginManager *plugins,
                               QWidget *parent)
    : QDialog(parent), m_plugins(plugins)
{
    setWindowTitle(tr("Preferences"));
    setObjectName(QStringLiteral("settingsDialog"));
    resize(760, 520);

    auto *root = new QHBoxLayout(this);
    auto *categories = new QListWidget(this);
    categories->setObjectName(QStringLiteral("settingsCategoryList"));
    categories->setFixedWidth(160);
    auto *pages = new QStackedWidget(this);

    auto addPage = [&](const QString &title, QWidget *page) {
        categories->addItem(title);
        pages->addWidget(page);
    };

    {
        auto *form = new QFormLayout;
        m_autoSave = new QCheckBox(tr("Automatically save modified files"), this);
        m_autoSaveDelay = new QSpinBox(this);
        m_autoSaveDelay->setRange(500, 60000);
        m_autoSaveDelay->setSingleStep(500);
        m_autoSaveDelay->setSuffix(tr(" ms"));
        form->addRow(m_autoSave);
        form->addRow(tr("Auto-save delay"), m_autoSaveDelay);
        addPage(tr("General"), wrapForm(form));
    }

    {
        auto *form = new QFormLayout;
        m_fontFamily = new QFontComboBox(this);
        m_fontSize = new QSpinBox(this);
        m_fontSize->setRange(8, 72);
        m_tabSize = new QSpinBox(this);
        m_tabSize->setRange(1, 16);
        m_insertSpaces = new QCheckBox(tr("Insert spaces instead of tabs"), this);
        m_wordWrap = new QCheckBox(tr("Word wrap"), this);
        m_lineNumbers = new QCheckBox(tr("Line numbers"), this);
        m_previewTabs = new QCheckBox(tr("Preview tabs (single-click in explorer)"), this);
        form->addRow(tr("Font"), m_fontFamily);
        form->addRow(tr("Font size"), m_fontSize);
        form->addRow(tr("Tab size"), m_tabSize);
        form->addRow(m_insertSpaces);
        form->addRow(m_wordWrap);
        form->addRow(m_lineNumbers);
        form->addRow(m_previewTabs);
        addPage(tr("Editor"), wrapForm(form));
    }

    {
        auto *form = new QFormLayout;
        m_warnMb = new QSpinBox(this);
        m_warnMb->setRange(1, 1024);
        m_warnMb->setSuffix(tr(" MB"));
        m_syntaxMb = new QSpinBox(this);
        m_syntaxMb->setRange(1, 2048);
        m_syntaxMb->setSuffix(tr(" MB"));
        m_hotExit = new QCheckBox(tr("Hot Exit (restore unsaved files after quit or crash)"), this);
        form->addRow(tr("Warn when opening larger than"), m_warnMb);
        form->addRow(tr("Disable syntax highlighting above"), m_syntaxMb);
        form->addRow(m_hotExit);
        addPage(tr("Files"), wrapForm(form));
    }

    {
        auto *page = new QWidget;
        auto *layout = new QVBoxLayout(page);
        layout->addWidget(new QLabel(tr("Excluded folder names (one per line)"), page));
        m_excluded = new QPlainTextEdit(page);
        layout->addWidget(m_excluded);
        addPage(tr("Workspace"), page);
    }

    {
        auto *form = new QFormLayout;
        m_shell = new QComboBox(this);
        m_shell->setEditable(true);
        m_shell->setInsertPolicy(QComboBox::NoInsert);
        m_shell->lineEdit()->setPlaceholderText(tr("Leave empty for the platform default"));
        for (const TerminalProfile &profile : detectShellProfiles()) {
            m_shell->addItem(QStringLiteral("%1 — %2").arg(profile.name,
                                                           QDir::toNativeSeparators(profile.shell)),
                             profile.shell);
        }
        m_usePty = new QCheckBox(tr("Use PTY (experimental, for ANSI colors)"), this);
        m_usePty->setEnabled(PtyTerminalBackend::isAvailable());
        form->addRow(tr("Shell"), m_shell);
        form->addRow(m_usePty);
        addPage(tr("Terminal"), wrapForm(form));
    }

    {
        auto *form = new QFormLayout;
        m_theme = new QComboBox(this);
        m_theme->addItems(ThemeManager::availableThemeIds());
        m_uiLanguage = new QComboBox(this);
        m_uiLanguage->addItem(tr("System"), QString());
        m_uiLanguage->addItem(tr("English"), QStringLiteral("en"));
        m_uiLanguage->addItem(tr("French"), QStringLiteral("fr"));
        auto *languageHint = new QLabel(tr("Language takes effect on the next launch."), this);
        languageHint->setWordWrap(true);
        form->addRow(tr("Theme"), m_theme);
        form->addRow(tr("Language"), m_uiLanguage);
        form->addRow(languageHint);
        addPage(tr("Appearance"), wrapForm(form));
    }

    m_keybindings = new KeybindingEditor(keybindings, commands, this);
    addPage(tr("Keyboard"), m_keybindings);

    {
        auto *page = new QWidget;
        auto *layout = new QVBoxLayout(page);
        layout->addWidget(
            new QLabel(tr("Local plugins: user folder and {workspace}/.editerako/plugins. "
                          "Uncheck to disable."),
                       page));
        m_pluginList = new QListWidget(page);
        m_pluginList->setObjectName(QStringLiteral("settingsPluginList"));
        layout->addWidget(m_pluginList, 1);
        auto *buttons = new QHBoxLayout;
        auto *openFolder = new QPushButton(tr("Open user plugins folder"), page);
        auto *reload = new QPushButton(tr("Reload plugins"), page);
        buttons->addWidget(openFolder);
        buttons->addWidget(reload);
        buttons->addStretch(1);
        layout->addLayout(buttons);
        connect(openFolder, &QPushButton::clicked, this, []() {
            const QString dir = PluginManager::userPluginsDirectory();
            QDir().mkpath(dir);
            QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
        });
        connect(reload, &QPushButton::clicked, this, [this]() {
            if (!m_plugins) {
                return;
            }
            QStringList disabled;
            for (int i = 0; i < m_pluginList->count(); ++i) {
                QListWidgetItem *item = m_pluginList->item(i);
                if (item && item->checkState() != Qt::Checked) {
                    disabled.append(item->data(Qt::UserRole).toString());
                }
            }
            AppSettings().setDisabledPlugins(disabled);
            m_plugins->setDisabledIds(disabled);
            m_plugins->reload();
            rebuildPluginList();
        });
        addPage(tr("Plugins"), page);
    }

    {
        auto *form = new QFormLayout;
        m_aiProvider = new QComboBox(this);
        for (const AiService &service : aiServices()) {
            const QString label = service.kind == AiService::Kind::Account
                                      ? tr("Sign in — %1").arg(service.name)
                                      : tr("API — %1").arg(service.name);
            m_aiProvider->addItem(label, service.id);
        }
        m_aiModel = new QLineEdit(this);
        m_aiEndpoint = new QLineEdit(this);
        form->addRow(tr("Chat"), m_aiProvider);
        form->addRow(tr("API model"), m_aiModel);
        form->addRow(tr("API endpoint"), m_aiEndpoint);
        form->addRow(new QLabel(
            tr("Account chat: sign in with your own ChatGPT, Claude, Gemini or Copilot session. "
               "API keys stay in the environment / .env file."),
            this));
        addPage(tr("AI"), wrapForm(form));
    }

    connect(categories, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);
    categories->setCurrentRow(0);

    root->addWidget(categories);
    auto *right = new QVBoxLayout;
    right->addWidget(pages, 1);
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    right->addWidget(buttons);
    root->addLayout(right, 1);

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        applyClicked();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Apply),
            &QPushButton::clicked,
            this,
            &SettingsDialog::applyClicked);

    load();
}

void SettingsDialog::load()
{
    const AppSettings settings;
    m_theme->setCurrentText(settings.themeId());
    const int languageIndex = m_uiLanguage->findData(settings.uiLanguage());
    m_uiLanguage->setCurrentIndex(languageIndex >= 0 ? languageIndex : 0);
    m_autoSave->setChecked(settings.autoSave());
    m_autoSaveDelay->setValue(settings.autoSaveDelayMs());
    m_fontFamily->setCurrentFont(QFont(settings.editorFontFamily()));
    m_fontSize->setValue(settings.editorFontSize());
    m_tabSize->setValue(settings.editorTabSize());
    m_insertSpaces->setChecked(settings.editorInsertSpaces());
    m_wordWrap->setChecked(settings.editorWordWrap());
    m_lineNumbers->setChecked(settings.editorLineNumbers());
    m_previewTabs->setChecked(settings.editorPreviewTabs());
    m_warnMb->setValue(bytesToMb(settings.largeFileWarnBytes()));
    m_syntaxMb->setValue(bytesToMb(settings.largeFileDisableSyntaxBytes()));
    m_hotExit->setChecked(settings.hotExit());
    m_excluded->setPlainText(settings.excludedFolders().join(QLatin1Char('\n')));
    const QString shell = settings.terminalShell();
    const int shellIndex = m_shell->findData(shell);
    if (shellIndex >= 0) {
        m_shell->setCurrentIndex(shellIndex);
    } else {
        m_shell->setCurrentIndex(-1);
        m_shell->setEditText(shell);
    }
    m_usePty->setChecked(settings.terminalUsePty());
    const int aiIndex = m_aiProvider->findData(settings.aiProvider());
    m_aiProvider->setCurrentIndex(aiIndex >= 0 ? aiIndex : 0);
    m_aiModel->setText(settings.aiModel());
    m_aiEndpoint->setText(settings.aiEndpoint());
    rebuildPluginList();
}

void SettingsDialog::rebuildPluginList()
{
    if (!m_pluginList) {
        return;
    }
    m_pluginList->clear();
    if (!m_plugins) {
        auto *item = new QListWidgetItem(tr("No plugin manager"));
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        m_pluginList->addItem(item);
        return;
    }
    const QVector<PluginInfo> plugins = m_plugins->plugins();
    if (plugins.isEmpty()) {
        auto *item = new QListWidgetItem(tr("No local plugins found"));
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        m_pluginList->addItem(item);
        return;
    }
    for (const PluginInfo &plugin : plugins) {
        QString source = tr("user");
        if (plugin.source == PluginInfo::Source::Workspace) {
            source = tr("workspace");
        } else if (plugin.source == PluginInfo::Source::InProcess) {
            source = tr("built-in");
        }
        QString text = QStringLiteral("%1  %2  (%3)").arg(plugin.name, plugin.version, source);
        if (!plugin.error.isEmpty()) {
            text += QStringLiteral(" — ") + plugin.error;
        }
        auto *item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, plugin.id);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(plugin.enabled ? Qt::Checked : Qt::Unchecked);
        m_pluginList->addItem(item);
    }
}

void SettingsDialog::save()
{
    AppSettings settings;
    settings.setThemeId(m_theme->currentText());
    settings.setUiLanguage(m_uiLanguage->currentData().toString());
    settings.setAutoSave(m_autoSave->isChecked());
    settings.setAutoSaveDelayMs(m_autoSaveDelay->value());
    settings.setEditorFontFamily(m_fontFamily->currentFont().family());
    settings.setEditorFontSize(m_fontSize->value());
    settings.setEditorTabSize(m_tabSize->value());
    settings.setEditorInsertSpaces(m_insertSpaces->isChecked());
    settings.setEditorWordWrap(m_wordWrap->isChecked());
    settings.setEditorLineNumbers(m_lineNumbers->isChecked());
    settings.setEditorPreviewTabs(m_previewTabs->isChecked());
    settings.setLargeFileWarnBytes(mbToBytes(m_warnMb->value()));
    settings.setLargeFileDisableSyntaxBytes(mbToBytes(m_syntaxMb->value()));
    settings.setHotExit(m_hotExit->isChecked());

    QStringList folders;
    const QStringList lines = m_excluded->toPlainText().split(QLatin1Char('\n'));
    for (QString line : lines) {
        line = line.trimmed();
        if (!line.isEmpty()) {
            folders.append(line);
        }
    }
    settings.setExcludedFolders(folders);
    const int shellIndex = m_shell->currentIndex();
    QString shell;
    if (shellIndex >= 0 && m_shell->currentText() == m_shell->itemText(shellIndex)) {
        shell = m_shell->itemData(shellIndex).toString();
    } else {
        shell = m_shell->currentText().trimmed();
    }
    settings.setTerminalShell(shell);
    settings.setTerminalUsePty(m_usePty->isChecked());
    settings.setAiProvider(m_aiProvider->currentData().toString());
    settings.setAiModel(m_aiModel->text().trimmed());
    settings.setAiEndpoint(m_aiEndpoint->text().trimmed());

    QStringList disabled;
    if (m_pluginList) {
        for (int i = 0; i < m_pluginList->count(); ++i) {
            QListWidgetItem *item = m_pluginList->item(i);
            if (item && item->data(Qt::UserRole).isValid() && item->checkState() != Qt::Checked) {
                disabled.append(item->data(Qt::UserRole).toString());
            }
        }
    }
    const QStringList previous = settings.disabledPlugins();
    settings.setDisabledPlugins(disabled);
    if (m_plugins && disabled != previous) {
        m_plugins->setDisabledIds(disabled);
        m_plugins->reload();
    }
}

void SettingsDialog::applyClicked()
{
    save();
    emit settingsApplied();
}
