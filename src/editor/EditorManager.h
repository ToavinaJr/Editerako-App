#ifndef EDITERAKO_EDITORMANAGER_H
#define EDITERAKO_EDITORMANAGER_H

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

class CodeEditor;
class EditorDocument;
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

    [[nodiscard]] QTabWidget *tabWidget() const { return m_tabs; }

    [[nodiscard]] CodeEditor *currentEditor() const;
    [[nodiscard]] EditorDocument *currentDocument() const;
    [[nodiscard]] QWidget *currentWidget() const;
    [[nodiscard]] QString currentFilePath() const;

    void setWorkingDirectory(const QString &dir);
    [[nodiscard]] QString workingDirectory() const { return m_workingDirectory; }

    CodeEditor *openUntitled();
    bool openTextFile(const QString &filePath);
    bool activateExisting(const QString &filePath);
    void addViewerTab(QWidget *widget, const QString &filePath);

    [[nodiscard]] QStringList openFilePaths() const;
    [[nodiscard]] CodeEditor *editorForPath(const QString &filePath) const;
    bool reloadFromDisk(const QString &filePath);
    void closeUntitledIfPristine();

    bool save(CodeEditor *editor);
    bool saveAs(CodeEditor *editor);
    bool saveCurrent();
    bool saveCurrentAs();
    bool saveAll();

    CloseResult closeTab(int index);
    CloseResult closeCurrent();
    CloseResult closeOthers();
    CloseResult closeAll();

    bool promptSaveAllOnQuit();

signals:
    void currentChanged();
    void modificationChanged();
    void aboutToSave(const QString &path);
    void fileSaved(const QString &path);

private:
    CodeEditor *createEditor();
    void updateTabLabel(CodeEditor *editor);
    [[nodiscard]] QString pathForWidget(QWidget *widget) const;
    [[nodiscard]] QList<CodeEditor *> modifiedEditors() const;
    bool confirmClose(CodeEditor *editor);
    bool writeToDisk(CodeEditor *editor, const QString &path);

    QWidget *m_dialogParent = nullptr;
    QTabWidget *m_tabs = nullptr;
    QString m_workingDirectory;
};

#endif
