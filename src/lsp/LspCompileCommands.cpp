#include "lsp/LspCompileCommands.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QPair>
#include <QVector>
#include <algorithm>

namespace {

void consider(const QString &dir, QVector<QPair<QDateTime, QString>> *candidates)
{
    if (!candidates || dir.isEmpty()) {
        return;
    }
    const QFileInfo info(QDir(dir).filePath(QStringLiteral("compile_commands.json")));
    if (!info.isFile()) {
        return;
    }
    candidates->append({info.lastModified(), QDir(dir).absolutePath()});
}

QString newestInTree(const QString &root)
{
    QVector<QPair<QDateTime, QString>> candidates;
    consider(root, &candidates);

    const QDir rootDir(root);
    consider(rootDir.filePath(QStringLiteral("build")), &candidates);
    consider(rootDir.filePath(QStringLiteral("cmake-build-debug")), &candidates);
    consider(rootDir.filePath(QStringLiteral("cmake-build-release")), &candidates);

    const QDir buildDir(rootDir.filePath(QStringLiteral("build")));
    if (buildDir.exists()) {
        const QFileInfoList subs = buildDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &sub : subs) {
            consider(sub.absoluteFilePath(), &candidates);
        }
    }

    if (candidates.isEmpty()) {
        return {};
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const QPair<QDateTime, QString> &a, const QPair<QDateTime, QString> &b) {
                  if (a.first != b.first) {
                      return a.first > b.first;
                  }
                  return a.second < b.second;
              });
    return candidates.front().second;
}

} // namespace

QString lspCompileCommandsDir(const QString &workspaceRoot)
{
    if (workspaceRoot.trimmed().isEmpty()) {
        return {};
    }

    const QFileInfo anchor(workspaceRoot);
    QString current = QDir(anchor.isDir() ? anchor.absoluteFilePath() : anchor.absolutePath())
                          .absolutePath();
    if (current.isEmpty()) {
        return {};
    }

    const bool walkUp = !anchor.exists() || !anchor.isDir();
    const int maxDepth = walkUp ? 12 : 1;
    for (int depth = 0; depth < maxDepth; ++depth) {
        const QString found = newestInTree(current);
        if (!found.isEmpty()) {
            return found;
        }
        QDir dir(current);
        if (!dir.cdUp()) {
            break;
        }
        const QString parent = dir.absolutePath();
        if (parent == current) {
            break;
        }
        current = parent;
    }
    return {};
}
