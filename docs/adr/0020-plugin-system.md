# ADR 0020 — Plugins locaux

- **Statut :** accepté
- **Date :** 2026-09-04
- **Phase :** 20

## Contexte

Le cahier des charges demande une API plugin locale stable (`IPlugin`, `PluginManager`, `PluginContext`), des contributions (command, menu, viewer, language, AI provider, panel), le système de plugins Qt si pertinent, et un éditeur qui fonctionne **sans** aucun plugin. Pas de marketplace. Le brief citait `0004-plugin-system.md` ; `0004` est déjà pris (encoding/EOL) → **0020**.

Le risque principal est une API trop large trop tôt.

## Décision

- Module **`EditerakoPlugins`** : `PluginManifest`, `PluginContext`, `IPlugin` (IID `org.editerako.IPlugin/1.0`), `IFileViewerProvider`, `PluginManager`.
- Chargement : dossiers utilisateur + workspace. Manifest JSON obligatoire. Binaire natif optionnel via `QPluginLoader`.
- Tests et plugins internes : `addInProcessPlugin` sans DLL.
- Contributions manifest : commandes (`CommandRegistry`), langages extra (`LanguageRegistry::setExtraLanguages`, ouverture texte), viewers/panneaux placeholder jusqu’à un `IPlugin` natif.
- `MainWindow` reste le composition root : handlers viewers / tabs, menu Plugins, Préférences.
- Désactivation : `plugins/disabled` (QSettings). Pas de scripts arbitraires, pas de `library` hors du dossier du plugin.

## Conséquences

- Un plugin JSON seul n’exécute pas de code natif.
- L’API `IPlugin` v1 est volontairement petite ; marketplace / grammaires tree-sitter plugin / factories AI ChatWidget : hors phase.
- `EditerakoPlugins` dépend de Core + Syntax (langages extra), pas de MainWindow.
