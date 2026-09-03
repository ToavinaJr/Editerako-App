#ifndef EDITERAKO_WORKSPACESEARCHDIALOG_H
#define EDITERAKO_WORKSPACESEARCHDIALOG_H

#include "project/WorkspaceSearch.h"

#include <QDialog>
#include <QList>

class EditorManager;
class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class WorkspaceController;

class WorkspaceSearchDialog : public QDialog
{
    Q_OBJECT

public:
    WorkspaceSearchDialog(WorkspaceController *workspace,
                          EditorManager *editors,
                          QWidget *parent = nullptr);

signals:
    void openHitRequested(const QString &path, int line, int column);
    void fileMutated(const QString &path);

private:
    void startSearch();
    void onResults(const QList<SearchHit> &hits);
    void onFinished(const SearchJobResult &result);
    void onSelectionChanged();
    void openCurrent();
    void replaceCurrent();
    void replaceAll();
    [[nodiscard]] SearchOptions currentOptions() const;
    [[nodiscard]] QHash<QString, QString> snapshotBuffers() const;
    bool applyReplacement(const QString &path, const QString &replacement, int *count);
    void rebuildTree();
    [[nodiscard]] SearchHit currentHit() const;

    WorkspaceController *m_workspace = nullptr;
    EditorManager *m_editors = nullptr;
    WorkspaceSearch *m_search = nullptr;

    QLineEdit *m_query = nullptr;
    QLineEdit *m_replace = nullptr;
    QLineEdit *m_include = nullptr;
    QLineEdit *m_exclude = nullptr;
    QCheckBox *m_caseSensitive = nullptr;
    QCheckBox *m_wholeWord = nullptr;
    QCheckBox *m_regex = nullptr;
    QPushButton *m_searchButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QPushButton *m_replaceButton = nullptr;
    QPushButton *m_replaceAllButton = nullptr;
    QTreeWidget *m_tree = nullptr;
    QPlainTextEdit *m_preview = nullptr;
    QLabel *m_status = nullptr;

    QList<SearchHit> m_hits;
};

#endif
