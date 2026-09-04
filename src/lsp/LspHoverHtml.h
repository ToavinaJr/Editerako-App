#ifndef EDITERAKO_LSPHOVERHTML_H
#define EDITERAKO_LSPHOVERHTML_H

#include <QString>

[[nodiscard]] QString lspEscapeHtml(const QString &text);
[[nodiscard]] QString lspHighlightCppHtml(const QString &code);
[[nodiscard]] QString lspMarkupToHtml(const QString &markdown, bool withDefinitionLink = false);

#endif
