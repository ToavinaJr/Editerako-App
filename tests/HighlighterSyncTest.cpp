#include "editor/HighlighterSync.h"

#include <QtTest>

class HighlighterSyncTest : public QObject
{
    Q_OBJECT

private slots:
    void pythonHighlightsWhenSmall();
    void cppBelowThresholdHighlights();
    void cppAboveThresholdSkipped();
    void htmlHighlightsWhenSmall();
};

void HighlighterSyncTest::pythonHighlightsWhenSmall()
{
    QVERIFY(HighlighterSync::shouldHighlight(LanguageId::Python, 100, 1000));
}

void HighlighterSyncTest::cppBelowThresholdHighlights()
{
    QVERIFY(HighlighterSync::shouldHighlight(LanguageId::Cpp, 100, 1000));
}

void HighlighterSyncTest::cppAboveThresholdSkipped()
{
    QVERIFY(!HighlighterSync::shouldHighlight(LanguageId::Cpp, 2000, 1000));
}

void HighlighterSyncTest::htmlHighlightsWhenSmall()
{
    QVERIFY(HighlighterSync::shouldHighlight(LanguageId::Html, 1, 1000));
}

QTEST_GUILESS_MAIN(HighlighterSyncTest)
#include "HighlighterSyncTest.moc"
