#include "terminal/CommandHistory.h"

void CommandHistory::add(const QString &command)
{
    const QString trimmed = command.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    if (m_entries.isEmpty() || m_entries.last() != trimmed) {
        m_entries.append(trimmed);
    }
    m_index = m_entries.size();
}

CommandHistory::Navigation CommandHistory::navigate(int direction)
{
    Navigation result;
    if (m_entries.isEmpty()) {
        return result;
    }

    m_index += direction;
    if (m_index < 0) {
        m_index = 0;
    } else if (m_index >= m_entries.size()) {
        m_index = m_entries.size();
        result.applied = true;
        result.clearLine = true;
        return result;
    }

    result.applied = true;
    result.command = m_entries.at(m_index);
    return result;
}
