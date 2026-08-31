#ifndef EDITERAKO_COMMANDDISCOVERY_H
#define EDITERAKO_COMMANDDISCOVERY_H

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>

class CommandDiscovery : public QObject
{
    Q_OBJECT

public:
    explicit CommandDiscovery(QObject *parent = nullptr);

    void initialize();
    void seedBuiltins();

    [[nodiscard]] QStringList commands() const { return m_commands; }
    [[nodiscard]] bool hasArguments(const QString &command) const;
    [[nodiscard]] QStringList arguments(const QString &command) const;
    void ensureArguments(const QString &command);

private:
    void loadCommandCache();
    void saveCommandCache();
    QStringList loadCachedArguments(const QString &command);
    void saveCachedArguments(const QString &command);
    void scanSystemCommandsAsync();
    void scanCommandArgumentsAsync(const QString &command);
    [[nodiscard]] static QString cacheDirectory();
    [[nodiscard]] static QString sanitizedCacheName(const QString &command);

    QStringList m_commands;
    QMap<QString, QStringList> m_arguments;
};

#endif
