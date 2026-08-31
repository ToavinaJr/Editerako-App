#ifndef EDITERAKO_COMMANDHISTORY_H
#define EDITERAKO_COMMANDHISTORY_H

#include <QString>
#include <QStringList>

class CommandHistory
{
public:
    struct Navigation {
        bool applied = false;
        bool clearLine = false;
        QString command;
    };

    void add(const QString &command);
    [[nodiscard]] Navigation navigate(int direction);

    [[nodiscard]] const QStringList &entries() const { return m_entries; }
    [[nodiscard]] bool isEmpty() const { return m_entries.isEmpty(); }
    [[nodiscard]] int size() const { return m_entries.size(); }

private:
    QStringList m_entries;
    int m_index = -1;
};

#endif
