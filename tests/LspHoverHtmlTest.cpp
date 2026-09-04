#include "lsp/LspHoverHtml.h"

#include <QtTest>

class LspHoverHtmlTest : public QObject
{
    Q_OBJECT
private slots:
    void escapesTemplateArguments();
    void highlightsKeywordsAndTypes();
    void includesDefinitionLink();
};

void LspHoverHtmlTest::escapesTemplateArguments()
{
    const QString html = lspMarkupToHtml(
        QStringLiteral("function pickerItemsFromRows\n"
                       "-> `QList<FuzzyPickerItem>`\n"
                       "- `const QVector<LspPickerRow> &rows`"),
        false);
    QVERIFY(html.contains(QStringLiteral("QList")));
    QVERIFY(html.contains(QStringLiteral("&lt;")));
    QVERIFY(html.contains(QStringLiteral("FuzzyPickerItem")));
    QVERIFY(html.contains(QStringLiteral("LspPickerRow")));
    QVERIFY(html.contains(QStringLiteral("rows")));
    QVERIFY(!html.contains(QStringLiteral("<FuzzyPickerItem>")));
}

void LspHoverHtmlTest::highlightsKeywordsAndTypes()
{
    const QString html = lspHighlightCppHtml(
        QStringLiteral("QList<FuzzyPickerItem> pickerItemsFromRows(const QVector<LspPickerRow> &rows)"));
    QVERIFY(html.contains(QStringLiteral("class=\"ht\"")));
    QVERIFY(html.contains(QStringLiteral("class=\"hk\"")));
    QVERIFY(html.contains(QStringLiteral("class=\"hf\"")));
    QVERIFY(html.contains(QStringLiteral("&lt;")));
}

void LspHoverHtmlTest::includesDefinitionLink()
{
    const QString html = lspMarkupToHtml(QStringLiteral("int foo()"), true);
    QVERIFY(html.contains(QStringLiteral("editerako:definition")));
    QVERIFY(html.contains(QStringLiteral("Go to Definition")));
}

QTEST_GUILESS_MAIN(LspHoverHtmlTest)
#include "LspHoverHtmlTest.moc"
