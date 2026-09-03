#include "lsp/LspMessageFramer.h"

#include <QByteArrayList>

void LspMessageFramer::append(const QByteArray &bytes)
{
    m_buffer.append(bytes);
}

void LspMessageFramer::clear()
{
    m_buffer.clear();
}

QByteArray LspMessageFramer::frame(const QByteArray &jsonBody)
{
    QByteArray header = "Content-Length: " + QByteArray::number(jsonBody.size()) + "\r\n\r\n";
    header.append(jsonBody);
    return header;
}

bool LspMessageFramer::takeMessage(QByteArray *jsonOut)
{
    if (!jsonOut) {
        return false;
    }

    int sep = m_buffer.indexOf("\r\n\r\n");
    int sepLen = 4;
    if (sep < 0) {
        sep = m_buffer.indexOf("\n\n");
        sepLen = 2;
    }
    if (sep < 0) {
        return false;
    }

    const QByteArray header = m_buffer.left(sep);
    int contentLength = -1;
    const QByteArrayList lines = header.split('\n');
    for (QByteArray line : lines) {
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        const int colon = line.indexOf(':');
        if (colon <= 0) {
            continue;
        }
        QByteArray name = line.left(colon).trimmed().toLower();
        if (name == "content-length") {
            bool ok = false;
            contentLength = line.mid(colon + 1).trimmed().toInt(&ok);
            if (!ok) {
                contentLength = -1;
            }
        }
    }

    if (contentLength < 0 || contentLength > maxMessageBytes) {
        m_buffer.remove(0, sep + sepLen);
        return false;
    }

    const int total = sep + sepLen + contentLength;
    if (m_buffer.size() < total) {
        return false;
    }

    *jsonOut = m_buffer.mid(sep + sepLen, contentLength);
    m_buffer.remove(0, total);
    return true;
}
