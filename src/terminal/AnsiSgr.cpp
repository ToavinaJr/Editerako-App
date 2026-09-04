#include "terminal/AnsiSgr.h"

#include <QChar>

namespace {

QColor defaultFg()
{
    return QColor(204, 204, 204);
}

} // namespace

AnsiSgrDecoder::AnsiSgrDecoder()
    : m_color(defaultFg())
    , m_defaultColor(defaultFg())
{
}

void AnsiSgrDecoder::reset()
{
    m_state = State::Text;
    m_csi.clear();
    m_bold = false;
    m_color = m_defaultColor;
}

QColor AnsiSgrDecoder::colorForCode(int code, bool bright)
{
    static const QColor normal[] = {
        QColor(40, 44, 52),    QColor(224, 108, 117), QColor(152, 195, 121), QColor(229, 192, 123),
        QColor(97, 175, 239),  QColor(198, 120, 221), QColor(86, 182, 194),  QColor(171, 178, 191),
    };
    static const QColor brightColors[] = {
        QColor(92, 99, 112),   QColor(239, 89, 111),  QColor(166, 226, 46),  QColor(230, 219, 116),
        QColor(102, 217, 239), QColor(174, 129, 255), QColor(161, 239, 228), QColor(248, 248, 242),
    };
    if (code < 0 || code > 7) {
        return defaultFg();
    }
    return bright ? brightColors[code] : normal[code];
}

void AnsiSgrDecoder::applySgr(const QString &params)
{
    const QStringList parts = params.split(QLatin1Char(';'));
    if (parts.isEmpty() || (parts.size() == 1 && parts.front().isEmpty())) {
        m_bold = false;
        m_color = m_defaultColor;
        return;
    }
    for (const QString &part : parts) {
        bool ok = false;
        const int code = part.toInt(&ok);
        if (!ok) {
            continue;
        }
        if (code == 0) {
            m_bold = false;
            m_color = m_defaultColor;
        } else if (code == 1) {
            m_bold = true;
        } else if (code >= 30 && code <= 37) {
            m_color = colorForCode(code - 30, m_bold);
        } else if (code >= 90 && code <= 97) {
            m_color = colorForCode(code - 90, true);
        } else if (code == 39) {
            m_color = m_defaultColor;
        }
    }
}

QVector<AnsiFragment> AnsiSgrDecoder::feed(const QString &chunk)
{
    QVector<AnsiFragment> out;
    QString current;
    auto flush = [&]() {
        if (!current.isEmpty()) {
            out.append(AnsiFragment{current, m_color});
            current.clear();
        }
    };

    for (const QChar ch : chunk) {
        switch (m_state) {
        case State::Text:
            if (ch == QChar(0x1b)) {
                flush();
                m_state = State::Esc;
            } else {
                current.append(ch);
            }
            break;
        case State::Esc:
            if (ch == QLatin1Char('[')) {
                m_csi.clear();
                m_state = State::Csi;
            } else {
                m_state = State::Text;
            }
            break;
        case State::Csi:
            if (ch.isLetter()) {
                if (ch == QLatin1Char('m')) {
                    applySgr(m_csi);
                }
                m_csi.clear();
                m_state = State::Text;
            } else {
                m_csi.append(ch);
            }
            break;
        }
    }
    flush();
    return out;
}

QString stripAnsi(const QString &text)
{
    AnsiSgrDecoder decoder;
    QString out;
    for (const AnsiFragment &frag : decoder.feed(text)) {
        out += frag.text;
    }
    return out;
}
