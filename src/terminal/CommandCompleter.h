#ifndef EDITERAKO_COMMANDCOMPLETER_H
#define EDITERAKO_COMMANDCOMPLETER_H

#include <QString>
#include <QStringList>

class CommandDiscovery;
class CommandHistory;

class CommandCompleter
{
public:
    CommandCompleter(CommandDiscovery *discovery, const CommandHistory *history);

    [[nodiscard]] QStringList suggest(const QString &currentLine, const QString &workingDirectory);
    [[nodiscard]] QStringList commandSuggestions(const QString &partial) const;
    [[nodiscard]] QStringList argumentSuggestions(const QString &command, const QString &partial) const;
    [[nodiscard]] static QStringList pathSuggestions(const QString &partial,
                                                     const QString &workingDirectory);

private:
    CommandDiscovery *m_discovery = nullptr;
    const CommandHistory *m_history = nullptr;
};

#endif
