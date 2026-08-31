#ifndef EDITERAKO_SESSIONSTORE_H
#define EDITERAKO_SESSIONSTORE_H

#include <QByteArray>
#include <QString>
#include <QStringList>

class QSettings;

struct SessionState {
    QString workspace;
    QStringList openFiles;
    QString activeFile;
    QByteArray geometry;
    QByteArray windowState;
};

class SessionStore
{
public:
    [[nodiscard]] SessionState load() const;
    void save(const SessionState &state);

    [[nodiscard]] SessionState load(QSettings &settings) const;
    void save(const SessionState &state, QSettings &settings);
};

#endif
