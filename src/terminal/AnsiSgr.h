#ifndef EDITERAKO_ANSISGR_H
#define EDITERAKO_ANSISGR_H

#include <QColor>
#include <QString>
#include <QVector>

struct AnsiFragment {
    QString text;
    QColor color;
};

class AnsiSgrDecoder
{
public:
    AnsiSgrDecoder();

    void reset();
    [[nodiscard]] QVector<AnsiFragment> feed(const QString &chunk);
    [[nodiscard]] QColor currentColor() const { return m_color; }

private:
    void applySgr(const QString &params);
    [[nodiscard]] static QColor colorForCode(int code, bool bright);

    enum class State { Text, Esc, Csi };

    State m_state = State::Text;
    QString m_csi;
    QColor m_color;
    QColor m_defaultColor;
    bool m_bold = false;
};

[[nodiscard]] QString stripAnsi(const QString &text);

#endif
