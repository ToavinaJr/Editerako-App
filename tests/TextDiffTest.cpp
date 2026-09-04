#include "scm/TextDiff.h"

#include <QtTest>

class TextDiffTest : public QObject
{
    Q_OBJECT
private slots:
    void identicalHasNoUnified();
    void insertionAndDeletion();
    void splitLinesNormalizesEol();
};

void TextDiffTest::identicalHasNoUnified()
{
    const QString text = QStringLiteral("a\nb\n");
    QVERIFY(TextDiff::unified(text, text).isEmpty());
    const QVector<DiffLine> lines = TextDiff::diffLines(text, text);
    QCOMPARE(lines.size(), 2);
    QCOMPARE(lines.at(0).kind, DiffLine::Kind::Equal);
}

void TextDiffTest::insertionAndDeletion()
{
    const QString left = QStringLiteral("keep\nremove\nend");
    const QString right = QStringLiteral("keep\nadd\nend");
    const QString unified = TextDiff::unified(left, right, QStringLiteral("a"), QStringLiteral("b"));
    QVERIFY(unified.contains(QStringLiteral("--- a")));
    QVERIFY(unified.contains(QStringLiteral("+++ b")));
    QVERIFY(unified.contains(QStringLiteral("-remove")));
    QVERIFY(unified.contains(QStringLiteral("+add")));

    const QVector<DiffLine> lines = TextDiff::diffLines(left, right);
    int deletes = 0;
    int inserts = 0;
    for (const DiffLine &line : lines) {
        if (line.kind == DiffLine::Kind::Delete) {
            ++deletes;
        }
        if (line.kind == DiffLine::Kind::Insert) {
            ++inserts;
        }
    }
    QCOMPARE(deletes, 1);
    QCOMPARE(inserts, 1);
}

void TextDiffTest::splitLinesNormalizesEol()
{
    const QStringList lines = TextDiff::splitLines(QStringLiteral("a\r\nb\rc"));
    QCOMPARE(lines, (QStringList{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")}));
}

QTEST_GUILESS_MAIN(TextDiffTest)
#include "TextDiffTest.moc"
