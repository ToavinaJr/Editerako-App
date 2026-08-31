#ifndef EDITERAKO_EDITORDOCUMENT_H
#define EDITERAKO_EDITORDOCUMENT_H

#include <QObject>
#include <QString>

class CodeEditor;

class EditorDocument : public QObject
{
    Q_OBJECT

public:
    explicit EditorDocument(CodeEditor *editor);

    [[nodiscard]] static EditorDocument *fromEditor(CodeEditor *editor);
    [[nodiscard]] static QString normalizePath(const QString &path);

    [[nodiscard]] CodeEditor *editor() const { return m_editor; }

    [[nodiscard]] QString filePath() const { return m_filePath; }
    void setFilePath(const QString &path);

    [[nodiscard]] QString displayName() const;
    [[nodiscard]] bool isUntitled() const;
    [[nodiscard]] bool isModified() const;

signals:
    void filePathChanged(const QString &path);
    void modificationChanged(bool modified);

private:
    CodeEditor *m_editor = nullptr;
    QString m_filePath;
};

#endif
