#ifndef EDITERAKO_LSPSESSION_H
#define EDITERAKO_LSPSESSION_H

#include "editor/ProblemModel.h"

#include <QJsonObject>
#include <QObject>
#include <QPoint>
#include <QSet>
#include <QString>
#include <QVector>

class CodeEditor;
class CompletionPopup;
class EditorDocument;
class EditorManager;
class HoverPopup;
class LspServerManager;
class QTimer;
class QWidget;
struct CompletionItem;
struct LspDiagnostic;
struct LspLocation;
struct LspSymbol;

class LspSession : public QObject
{
    Q_OBJECT

public:
    LspSession(LspServerManager *manager, EditorManager *editors, QWidget *dialogParent);

    void setWorkspaceRoot(const QString &root);

public slots:
    void triggerCompletion();
    void triggerHover();
    void triggerSignatureHelp();
    void goToDefinition();
    void findReferences();
    void renameSymbol();
    void showDocumentSymbols();
    void showWorkspaceSymbols();

signals:
    void statusMessage(const QString &message, int timeoutMs);
    void lspStatusChanged(const QString &text);
    void problemsChanged(const QString &path, const QVector<ProblemItem> &items);

private:
    void onDocumentOpened(CodeEditor *editor);
    void onDocumentAboutToClose(CodeEditor *editor);
    void onFileSaved(const QString &path);
    void onInitialized(bool initialized);
    void onDiagnostics(const QString &uri, const QVector<LspDiagnostic> &diagnostics);
    void onEditorContentsChanged(CodeEditor *editor);
    void onHoverRequested(int line, int character, const QPoint &globalPos);
    void flushPendingChange();
    void attachEditor(CodeEditor *editor);
    bool ensureClangd(EditorDocument *doc);
    void openOnServer(CodeEditor *editor);
    void closeOnServer(const QString &path);
    [[nodiscard]] bool currentPosition(QString *uri, int *line, int *character) const;
    void applyCompletion(const CompletionItem &item);
    void applyWorkspaceEdits(const QJsonObject &edit);
    void goToLocations(const QVector<LspLocation> &locations, const QString &title);
    void showSymbols(const QVector<LspSymbol> &symbols, const QString &title);
    [[nodiscard]] QString languageKey(EditorDocument *doc) const;

    LspServerManager *m_manager = nullptr;
    EditorManager *m_editors = nullptr;
    QWidget *m_dialogParent = nullptr;
    CompletionPopup *m_completion = nullptr;
    HoverPopup *m_hover = nullptr;
    QTimer *m_changeTimer = nullptr;
    QString m_workspaceRoot;
    QSet<QString> m_openUris;
    CodeEditor *m_pendingChangeEditor = nullptr;
    int m_completionGeneration = 0;
    int m_hoverGeneration = 0;
    bool m_clangdRegistered = false;
    bool m_missingWarned = false;
};

#endif
