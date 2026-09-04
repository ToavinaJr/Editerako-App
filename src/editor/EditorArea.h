#ifndef EDITERAKO_EDITORAREA_H
#define EDITERAKO_EDITORAREA_H

#include <QList>
#include <QWidget>

class EditorGroup;
class QSplitter;
class QVBoxLayout;

class EditorArea : public QWidget
{
    Q_OBJECT

public:
    explicit EditorArea(QWidget *parent = nullptr);

    void setInitialGroup(EditorGroup *group);
    void split(EditorGroup *existing, EditorGroup *added, Qt::Orientation orientation);
    void removeGroup(EditorGroup *group);

    [[nodiscard]] QList<EditorGroup *> groups() const;
    [[nodiscard]] int groupCount() const { return groups().size(); }

private:
    void collectGroups(QWidget *node, QList<EditorGroup *> *out) const;
    void unwrapSplitter(QSplitter *splitter);
    [[nodiscard]] QWidget *rootWidget() const;

    QVBoxLayout *m_layout = nullptr;
};

#endif
