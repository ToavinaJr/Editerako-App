#ifndef EDITERAKO_WELCOMEDIALOG_H
#define EDITERAKO_WELCOMEDIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

class QListWidget;
class QPushButton;

class WelcomeDialog : public QDialog
{
    Q_OBJECT

public:
    enum Choice
    {
        Canceled,
        OpenFolder,
        OpenFile,
        OpenRecent
    };

    explicit WelcomeDialog(const QStringList &recents, QWidget *parent = nullptr);

    [[nodiscard]] Choice choice() const { return m_choice; }
    [[nodiscard]] QString selectedRecent() const { return m_selected; }

signals:
    void removeRequested(const QString &path);

private:
    void acceptRecent();
    void showListMenu(const QPoint &pos);

    QListWidget *m_list = nullptr;
    QPushButton *m_openRecent = nullptr;
    Choice m_choice = Canceled;
    QString m_selected;
};

#endif
