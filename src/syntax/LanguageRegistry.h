#ifndef EDITERAKO_LANGUAGEREGISTRY_H
#define EDITERAKO_LANGUAGEREGISTRY_H

#include <QString>

extern "C" {
struct TSLanguage;
}

enum class LanguageId {
    PlainText,
    Cpp,
    Html,
};

class LanguageRegistry
{
public:
    [[nodiscard]] static LanguageId idForPath(const QString &path);
    [[nodiscard]] static LanguageId idForExtension(const QString &extension);
    [[nodiscard]] static QString displayName(LanguageId id);
    [[nodiscard]] static const TSLanguage *tsLanguage(LanguageId id);
};

#endif
