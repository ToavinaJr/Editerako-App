#ifndef EDITERAKO_EDITORMANAGER_H
#define EDITERAKO_EDITORMANAGER_H

#include "core/BackupService.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

class CodeEditor;
class EditorArea;
class EditorDocument;
class EditorGroup;
class QTabWidget;
class QWidget;

class EditorManager : public QObject
{
    Q_OBJECT

public:
    enum class CloseResult {
        Closed,
        Cancelled,
    };

    explicit EditorManager(QWidget *dialogParent);

    [[nodiscard]] QWidget *containerWidget() const;
    [[nodiscard]] QTabWidget *tabWidget() const;
    [[nodiscard]] int activeTabCount() const;
    [[nodiscard]] int totalTabCount() const;
    [[nodiscard]] int groupCount() const;

    [[nodiscard]] CodeEditor *currentEditor() const;
    [[nodiscard]] EditorDocument *currentDocument() const;
    [[nodiscard]] QWidget *currentWidget() const;
    [[nodiscard]] QString currentFilePath() const;

    void setWorkingDirectory(const QString &dir);
    [[nodiscard]] QString workingDirectory() const { return m_workingDirectory; }

    CodeEditor *openUntitled();
    bool openTextFile(const QString &filePath, bool preview = false);
    bool activateExisting(const QString &filePath);
    bool activateWidget(QWidget *widget);
    void addViewerTab(QWidget *widget, const QString &filePath, bool preview = false);
    void promoteCurrentTab();
    void togglePinCurrentTab();
    void copyCurrentPath();
    void revealCurrentInOs();
    [[nodiscard]] QList<QWidget *> tabWidgets() const;

    [[nodiscard]] QStringList openFilePaths() const;
    [[nodiscard]] CodeEditor *editorForPath(const QString &filePath) const;
    [[nodiscard]] QList<CodeEditor *> editorsForPath(const QString &filePath) const;
    bool reloadFromDisk(const QString &filePath);
    void closeUntitledIfPristine();

    bool save(CodeEditor *editor);
    bool saveAs(CodeEditor *editor);
    bool saveCurrent();
    bool saveCurrentAs();
    bool saveAll();

    CloseResult closeTab(int index);
    CloseResult closeWidget(QWidget *widget);
    CloseResult closeCurrent();
    CloseResult closeOthers();
    CloseResult closeToRight();
    CloseResult closeSaved();
    CloseResult closeAll();

    void splitRight();
    void splitDown();
    void moveEditor();
    void closeActiveGroup();

    bool promptSaveAllOnQuit();
    bool promptSaveEditors(const QList<CodeEditor *> &editors, bool secretFiles = false);
    void applySettings();
    void setLineNumbersVisible(bool visible);
    void adjustFontSize(int delta);
    void resetFontSize();
    bool goToLine(int lineNumber, int column = 0);
    bool revealLocation(const QString &filePath, int line, int character);
    void saveDirtyFilesQuietly();
    [[nodiscard]] QList<CodeEditor *> editors() const;
    [[nodiscard]] QList<CodeEditor *> modifiedEditors() const;
    [[nodiscard]] QList<BackupBuffer> dirtyBuffers() const;
    bool restoreBuffer(const BackupBuffer &buffer);

signals:
    void currentChanged();
    void modificationChanged();
    void aboutToSave(const QString &path);
    void fileSaved(const QString &path);
    void documentOpened(CodeEditor *editor);
    void documentAboutToClose(CodeEditor *editor);
    void viewerDuplicateRequested(const QString &path);

private:
    CodeEditor *createEditor();
    CodeEditor *cloneEditorView(CodeEditor *source);
    EditorGroup *createGroup();
    void setActiveGroup(EditorGroup *group);
    void bindDocument(CodeEditor *editor, EditorDocument *doc);
    void updateTabLabel(CodeEditor *editor);
    void updateTabLabelsFor(EditorDocument *doc);
    void refreshTab(EditorGroup *group, int index);
    void refreshWidgetTab(QWidget *widget);
    void refreshGroupTabs(EditorGroup *group);
    void showTabContextMenu(EditorGroup *group, int index, const QPoint &globalPos);
    void split(Qt::Orientation orientation);
    void moveCurrentToGroup(EditorGroup *target);
    EditorGroup *groupForWidget(QWidget *widget) const;
    EditorGroup *nextGroup(EditorGroup *group) const;
    void pruneEmptyGroup(EditorGroup *group);
    [[nodiscard]] QString pathForWidget(QWidget *widget) const;
    [[nodiscard]] int viewCount(EditorDocument *doc) const;
    bool confirmClose(CodeEditor *editor);
    bool writeToDisk(CodeEditor *editor, const QString &path);
    CloseResult closeInGroup(EditorGroup *group, int index);
    CloseResult closeWidgets(const QList<QWidget *> &widgets);
    void replacePreviewIfNeeded(bool preview);
    void applyTabMode(int index, bool preview);

    QWidget *m_dialogParent = nullptr;
    EditorArea *m_area = nullptr;
    EditorGroup *m_active = nullptr;
    QString m_workingDirectory;
};

#endif
