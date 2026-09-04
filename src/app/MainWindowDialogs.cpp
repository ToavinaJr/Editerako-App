#include "app/MainWindow.h"

#include "core/CommandRegistry.h"
#include "editor/CodeEditor.h"
#include "editor/EditorManager.h"
#include "editor/FindReplaceDialog.h"
#include "editor/GoToLineDialog.h"
#include "plugins/PluginManager.h"
#include "project/WorkspaceController.h"
#include "ui/CommandPaletteDialog.h"
#include "ui/QuickOpenDialog.h"
#include "ui/SettingsDialog.h"
#include "ui/WorkspaceSearchDialog.h"

#include <QAction>
#include <QDialog>
#include <QMessageBox>

void MainWindow::openPreferences()
{
    SettingsDialog dialog(m_keybindings, m_commands, m_pluginManager, this);
    connect(&dialog, &SettingsDialog::settingsApplied, this, &MainWindow::applyPreferences);
    dialog.exec();
}

void MainWindow::openCommandPalette()
{
    CommandPaletteDialog dialog(m_commands, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QAction *action = m_commands ? m_commands->action(dialog.selectedId()) : nullptr;
    if (action && action->isEnabled()) {
        action->trigger();
    }
}

void MainWindow::openQuickOpen()
{
    if (!m_workspaceController) {
        return;
    }
    const QStringList extra = m_editorManager ? m_editorManager->openFilePaths() : QStringList{};
    QuickOpenDialog dialog(m_workspaceController->fileIndex(), extra, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const QString path = dialog.selectedPath();
    if (path.isEmpty()) {
        return;
    }
    openFileInEditor(path);
    if (dialog.selectedLine() > 0 && m_editorManager) {
        m_editorManager->goToLine(dialog.selectedLine());
    }
}

void MainWindow::openWorkspaceSearch()
{
    if (!m_workspaceController || !m_editorManager) {
        return;
    }
    if (!m_searchDialog) {
        m_searchDialog = new WorkspaceSearchDialog(m_workspaceController, m_editorManager, this);
        connect(m_searchDialog, &WorkspaceSearchDialog::openHitRequested, this,
                [this](const QString &path, int line, int column) {
                    openFileInEditor(path);
                    if (m_editorManager) {
                        m_editorManager->goToLine(line, column);
                    }
                });
        connect(m_searchDialog, &WorkspaceSearchDialog::fileMutated, this,
                [this](const QString &path) {
                    if (m_workspaceController) {
                        m_workspaceController->ignoreNextChange(path);
                        m_workspaceController->refreshIfContains(path);
                    }
                });
    }
    m_searchDialog->show();
    m_searchDialog->raise();
    m_searchDialog->activateWindow();
}

void MainWindow::onActionFindReplace()
{
    CodeEditor *ed = currentEditor();
    if (!ed) {
        QMessageBox::information(this, tr("Find"), tr("No text editor is active."));
        return;
    }
    FindReplaceDialog dlg(ed, this);
    dlg.exec();
}

void MainWindow::onActionGoToLine()
{
    CodeEditor *ed = currentEditor();
    if (!ed) {
        QMessageBox::information(this, tr("Go to Line"), tr("No text editor is active."));
        return;
    }
    GoToLineDialog dlg(ed, this);
    dlg.exec();
}
