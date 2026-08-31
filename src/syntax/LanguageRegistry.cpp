#include "syntax/LanguageRegistry.h"

#include <QFileInfo>

extern "C" {
struct TSLanguage;
TSLanguage *tree_sitter_cpp();
TSLanguage *tree_sitter_html();
}

LanguageId LanguageRegistry::idForPath(const QString &path)
{
    if (path.isEmpty()) {
        return LanguageId::PlainText;
    }
    return idForExtension(QFileInfo(path).suffix());
}

LanguageId LanguageRegistry::idForExtension(const QString &extension)
{
    const QString ext = extension.toLower();
    if (ext == QLatin1String("cpp") || ext == QLatin1String("cc") || ext == QLatin1String("cxx")
        || ext == QLatin1String("c++") || ext == QLatin1String("h") || ext == QLatin1String("hpp")
        || ext == QLatin1String("hh") || ext == QLatin1String("hxx") || ext == QLatin1String("c")) {
        return LanguageId::Cpp;
    }
    if (ext == QLatin1String("html") || ext == QLatin1String("htm")) {
        return LanguageId::Html;
    }
    return LanguageId::PlainText;
}

QString LanguageRegistry::displayName(LanguageId id)
{
    switch (id) {
    case LanguageId::Cpp:
        return QStringLiteral("C++");
    case LanguageId::Html:
        return QStringLiteral("HTML");
    case LanguageId::PlainText:
        break;
    }
    return QStringLiteral("Plain Text");
}

const TSLanguage *LanguageRegistry::tsLanguage(LanguageId id)
{
    switch (id) {
    case LanguageId::Cpp:
        return tree_sitter_cpp();
    case LanguageId::Html:
        return tree_sitter_html();
    case LanguageId::PlainText:
        break;
    }
    return nullptr;
}
