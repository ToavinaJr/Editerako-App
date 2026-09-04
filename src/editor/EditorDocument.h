#ifndef EDITERAKO_EDITORDOCUMENT_H
#define EDITERAKO_EDITORDOCUMENT_H

#include "core/TextFileFormat.h"
#include "syntax/LanguageRegistry.h"

#include <QObject>
#include <QString>

class CodeEditor;

class EditorDocument : public QObject
{
    Q_OBJECT

public:
    struct CaretState {
        int position = 0;
        int anchor = 0;
        int scrollY = 0;
    };

    explicit EditorDocument(CodeEditor *editor, QObject *parent = nullptr);

    [[nodiscard]] static EditorDocument *fromEditor(CodeEditor *editor);
    void setEditor(CodeEditor *editor);
    [[nodiscard]] static QString normalizePath(const QString &path);

    [[nodiscard]] CodeEditor *editor() const { return m_editor; }

    [[nodiscard]] QString filePath() const { return m_filePath; }
    void setFilePath(const QString &path);

    [[nodiscard]] QString displayName() const;
    [[nodiscard]] bool isUntitled() const;
    [[nodiscard]] bool isModified() const;

    [[nodiscard]] TextFileMeta format() const { return m_format; }
    void setFormat(const TextFileMeta &meta);

    [[nodiscard]] LanguageId language() const;
    [[nodiscard]] bool isReadOnly() const { return m_readOnly; }
    void setReadOnly(bool readOnly);

    [[nodiscard]] int version() const { return m_version; }
    void resetVersion();

    [[nodiscard]] CaretState caretState() const;
    void restoreCaretState(const CaretState &state);

    [[nodiscard]] QString backupId() const { return m_backupId; }
    void setBackupId(const QString &id);
    [[nodiscard]] QString ensureBackupId();

signals:
    void filePathChanged(const QString &path);
    void modificationChanged(bool modified);
    void formatChanged();
    void versionChanged(int version);

private:
    CodeEditor *m_editor = nullptr;
    QString m_filePath;
    QString m_backupId;
    TextFileMeta m_format;
    bool m_readOnly = false;
    int m_version = 0;
};

#endif
