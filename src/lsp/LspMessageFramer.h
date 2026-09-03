#ifndef EDITERAKO_LSPMESSAGEFRAMER_H
#define EDITERAKO_LSPMESSAGEFRAMER_H

#include <QByteArray>

class LspMessageFramer
{
public:
    static constexpr int maxMessageBytes = 32 * 1024 * 1024;

    void append(const QByteArray &bytes);
    [[nodiscard]] bool takeMessage(QByteArray *jsonOut);
    [[nodiscard]] static QByteArray frame(const QByteArray &jsonBody);
    void clear();
    [[nodiscard]] int bufferedBytes() const { return m_buffer.size(); }

private:
    QByteArray m_buffer;
};

#endif
