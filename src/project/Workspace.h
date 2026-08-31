#ifndef EDITERAKO_WORKSPACE_H
#define EDITERAKO_WORKSPACE_H

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

class Workspace : public QObject
{
    Q_OBJECT

public:
    struct Entry {
        QString name;
        QString absolutePath;
        bool isDirectory = false;
    };

    explicit Workspace(QObject *parent = nullptr);

    void setRootPath(const QString &path);
    [[nodiscard]] QString rootPath() const { return m_rootPath; }
    [[nodiscard]] bool isValid() const;

    [[nodiscard]] bool isExcludedName(const QString &name) const;
    [[nodiscard]] bool containsPath(const QString &filePath) const;
    [[nodiscard]] QList<Entry> listEntries(const QString &directoryPath) const;

    [[nodiscard]] static bool createEmptyFile(const QString &directory,
                                              const QString &fileName,
                                              QString *absolutePath = nullptr);
    [[nodiscard]] static bool createDirectory(const QString &directory,
                                              const QString &folderName,
                                              QString *absolutePath = nullptr);

signals:
    void rootPathChanged(const QString &path);

private:
    [[nodiscard]] QStringList excludedNames() const;

    QString m_rootPath;
};

#endif
