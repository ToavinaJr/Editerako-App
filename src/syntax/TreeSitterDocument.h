#ifndef EDITERAKO_TREESITTERDOCUMENT_H
#define EDITERAKO_TREESITTERDOCUMENT_H

#include "syntax/LanguageRegistry.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVector>
#include <functional>

#include <tree_sitter/api.h>

class QTextDocument;

class TreeSitterDocument : public QObject
{
    Q_OBJECT

public:
    TreeSitterDocument(QTextDocument *document, LanguageId language, QObject *parent = nullptr);
    ~TreeSitterDocument() override;

    [[nodiscard]] bool isReady() const;
    [[nodiscard]] LanguageId language() const { return m_language; }

    [[nodiscard]] bool utf8RangeForBlock(int blockNumber, uint32_t *start, uint32_t *end) const;
    [[nodiscard]] TSNode rootNode() const;
    [[nodiscard]] const QByteArray &utf8() const { return m_utf8; }
    void visitOverlapping(uint32_t startByte, uint32_t endByte,
                          const std::function<void(TSNode)> &visitor) const;

private:
    void parseFull();
    void rebuildLineStarts();
    [[nodiscard]] QString snapshotText() const;
    void onContentsChange(int position, int charsRemoved, int charsAdded);

    QTextDocument *m_document = nullptr;
    LanguageId m_language = LanguageId::PlainText;
    TSParser *m_parser = nullptr;
    TSTree *m_tree = nullptr;
    QString m_text;
    QByteArray m_utf8;
    QVector<uint32_t> m_lineStartBytes;
};

#endif
