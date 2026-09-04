#include "core/BackupService.h"

#include "core/AtomicFile.h"
#include "core/Logging.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>

namespace {

constexpr auto kIndexFile = "index.json";
constexpr auto kContentSuffix = ".txt";

QString encodingToToken(TextEncoding encoding)
{
    switch (encoding) {
    case TextEncoding::Utf16Le:
        return QStringLiteral("utf-16le");
    case TextEncoding::Utf16Be:
        return QStringLiteral("utf-16be");
    case TextEncoding::Latin1:
        return QStringLiteral("latin1");
    case TextEncoding::Utf8:
    default:
        return QStringLiteral("utf-8");
    }
}

TextEncoding encodingFromToken(const QString &token)
{
    if (token == QLatin1String("utf-16le")) {
        return TextEncoding::Utf16Le;
    }
    if (token == QLatin1String("utf-16be")) {
        return TextEncoding::Utf16Be;
    }
    if (token == QLatin1String("latin1")) {
        return TextEncoding::Latin1;
    }
    return TextEncoding::Utf8;
}

QString lineEndingToToken(LineEnding ending)
{
    switch (ending) {
    case LineEnding::CrLf:
        return QStringLiteral("crlf");
    case LineEnding::Cr:
        return QStringLiteral("cr");
    case LineEnding::Lf:
    default:
        return QStringLiteral("lf");
    }
}

LineEnding lineEndingFromToken(const QString &token)
{
    if (token == QLatin1String("crlf")) {
        return LineEnding::CrLf;
    }
    if (token == QLatin1String("cr")) {
        return LineEnding::Cr;
    }
    return LineEnding::Lf;
}

bool isSafeEntryId(const QString &id)
{
    static const QRegularExpression re(QStringLiteral(R"(^[0-9a-fA-F-]{8,64}$)"));
    return re.match(id).hasMatch();
}

QString indexPath(const QString &root)
{
    return QDir(root).filePath(QString::fromLatin1(kIndexFile));
}

QString contentPath(const QString &root, const QString &id)
{
    return QDir(root).filePath(id + QLatin1String(kContentSuffix));
}

} // namespace

QString BackupService::defaultRoot()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (base.isEmpty()) {
        return {};
    }
    return QDir(base).filePath(QStringLiteral("backups"));
}

bool BackupService::isSecretPath(const QString &path)
{
    if (path.isEmpty()) {
        return false;
    }

    const QString name = QFileInfo(path).fileName().toLower();
    if (name == QLatin1String(".env.example")) {
        return false;
    }
    if (name == QLatin1String(".env") || name.startsWith(QLatin1String(".env."))
        || name.endsWith(QLatin1String(".env"))) {
        return true;
    }
    if (name == QLatin1String(".envrc") || name == QLatin1String("token")
        || name == QLatin1String("token.save")) {
        return true;
    }
    if (name == QLatin1String("credentials.json") || name == QLatin1String("secrets.json")
        || name == QLatin1String("secret.json")) {
        return true;
    }
    if (name == QLatin1String("id_rsa") || name == QLatin1String("id_dsa")
        || name == QLatin1String("id_ecdsa") || name == QLatin1String("id_ed25519")) {
        return true;
    }
    if (name.endsWith(QLatin1String(".pem")) || name.endsWith(QLatin1String(".key"))
        || name.endsWith(QLatin1String(".p12")) || name.endsWith(QLatin1String(".pfx"))
        || name.endsWith(QLatin1String(".p8")) || name.endsWith(QLatin1String(".ppk"))) {
        return true;
    }
    return false;
}

bool BackupService::exceedsSizeLimit(const QString &lfText)
{
    return lfText.toUtf8().size() > kMaxContentBytes;
}

BackupService::BackupService(QString root)
    : m_root(root.isEmpty() ? defaultRoot() : std::move(root))
{
}

bool BackupService::hasIndex() const
{
    return QFileInfo::exists(indexPath(m_root));
}

bool BackupService::writeSnapshot(const BackupSnapshot &snapshot) const
{
    if (m_root.isEmpty()) {
        return false;
    }

    if (snapshot.entries.isEmpty()) {
        clear();
        return true;
    }

    if (!QDir().mkpath(m_root)) {
        qCWarning(lcCore) << "Could not create backup directory" << m_root;
        return false;
    }

    QJsonArray entriesJson;
    QStringList keepNames;
    keepNames.append(QString::fromLatin1(kIndexFile));

    for (const BackupBuffer &entry : snapshot.entries) {
        if (entry.id.isEmpty() || !isSafeEntryId(entry.id)) {
            qCWarning(lcCore) << "Skipping backup entry with invalid id";
            continue;
        }
        if (exceedsSizeLimit(entry.lfText)) {
            qCWarning(lcCore) << "Skipping oversized backup entry" << entry.displayName;
            continue;
        }
        if (isSecretPath(entry.originalPath)) {
            continue;
        }

        QString error;
        if (!writeTextAtomically(contentPath(m_root, entry.id), entry.lfText, &error)) {
            qCWarning(lcCore) << "Failed to write backup content" << error;
            return false;
        }

        QJsonObject object;
        object.insert(QStringLiteral("id"), entry.id);
        object.insert(QStringLiteral("originalPath"), entry.originalPath);
        object.insert(QStringLiteral("displayName"), entry.displayName);
        object.insert(QStringLiteral("encoding"), encodingToToken(entry.format.encoding));
        object.insert(QStringLiteral("lineEnding"), lineEndingToToken(entry.format.lineEnding));
        object.insert(QStringLiteral("bom"), entry.format.bom);
        object.insert(QStringLiteral("caretPosition"), entry.caretPosition);
        object.insert(QStringLiteral("caretAnchor"), entry.caretAnchor);
        entriesJson.append(object);
        keepNames.append(entry.id + QLatin1String(kContentSuffix));
    }

    if (entriesJson.isEmpty()) {
        clear();
        return true;
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("workspace"), snapshot.workspace);
    root.insert(QStringLiteral("entries"), entriesJson);

    QString error;
    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Compact);
    if (!writeBytesAtomically(indexPath(m_root), bytes, &error)) {
        qCWarning(lcCore) << "Failed to write backup index" << error;
        return false;
    }

    QDir dir(m_root);
    const QStringList files = dir.entryList(QDir::Files);
    for (const QString &name : files) {
        if (!keepNames.contains(name)) {
            dir.remove(name);
        }
    }
    return true;
}

BackupSnapshot BackupService::loadSnapshot() const
{
    BackupSnapshot snapshot;
    if (m_root.isEmpty()) {
        return snapshot;
    }

    QFile file(indexPath(m_root));
    if (!file.open(QIODevice::ReadOnly)) {
        return snapshot;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        qCWarning(lcCore) << "Invalid backup index" << file.fileName() << parseError.errorString();
        return snapshot;
    }

    const QJsonObject root = document.object();
    snapshot.workspace = root.value(QStringLiteral("workspace")).toString();
    const QJsonArray entries = root.value(QStringLiteral("entries")).toArray();
    snapshot.entries.reserve(entries.size());

    for (const QJsonValue &value : entries) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject object = value.toObject();
        BackupBuffer entry;
        entry.id = object.value(QStringLiteral("id")).toString();
        if (!isSafeEntryId(entry.id)) {
            continue;
        }

        QFile content(contentPath(m_root, entry.id));
        if (!content.open(QIODevice::ReadOnly)) {
            qCWarning(lcCore) << "Missing backup content" << entry.id;
            continue;
        }
        entry.lfText = QString::fromUtf8(content.readAll());
        entry.originalPath = object.value(QStringLiteral("originalPath")).toString();
        entry.displayName = object.value(QStringLiteral("displayName")).toString();
        entry.format.encoding = encodingFromToken(object.value(QStringLiteral("encoding")).toString());
        entry.format.lineEnding = lineEndingFromToken(object.value(QStringLiteral("lineEnding")).toString());
        entry.format.bom = object.value(QStringLiteral("bom")).toBool();
        entry.caretPosition = object.value(QStringLiteral("caretPosition")).toInt();
        entry.caretAnchor = object.value(QStringLiteral("caretAnchor")).toInt();
        snapshot.entries.append(entry);
    }
    return snapshot;
}

void BackupService::clear() const
{
    if (m_root.isEmpty()) {
        return;
    }

    QDir dir(m_root);
    if (!dir.exists()) {
        return;
    }

    const QStringList files = dir.entryList(QDir::Files);
    for (const QString &name : files) {
        dir.remove(name);
    }
}
