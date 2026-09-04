# Plugins locaux

Editerako charge des **plugins locaux** uniquement. Pas de marketplace, pas d’installation distante.

## Dossiers

| Source | Chemin |
|---|---|
| Utilisateur | `%AppData%/Editerako/plugins/<id>/` |
| Workspace | `{workspace}/.editerako/plugins/<id>/` |

Chaque plugin est un dossier contenant `plugin.json`. L’application démarre **sans aucun plugin**.

## Manifest (`plugin.json`)

```json
{
  "id": "example.hello",
  "name": "Hello",
  "version": "1.0.0",
  "apiVersion": 1,
  "library": "hello",
  "contributes": {
    "commands": [{ "id": "hello.greet", "title": "Hello: Greet" }],
    "languages": [{ "id": "toml", "displayName": "TOML", "extensions": [".toml"] }],
    "viewers": [{ "id": "hello.csv", "title": "CSV", "extensions": [".csv"] }],
    "aiProviders": [{ "id": "hello.ai", "title": "Hello AI" }],
    "panels": [{ "id": "hello.panel", "title": "Hello" }]
  }
}
```

`apiVersion` doit valoir `1`. `library` est optionnel : binaire natif dans le **même** dossier (`hello.dll` / `libhello.so` / `hello.dylib`). Les chemins `..` et absolus sont refusés.

Sans bibliothèque, les commandes apparaissent dans **Plugins** et la palette ; un clic affiche un message. Les viewers / panneaux sont des placeholders. Un langage extra ouvre le fichier comme du texte (pas de tree-sitter).

## API native (`IPlugin`)

```
org.editerako.IPlugin/1.0
```

Classe Qt : `QObject` + `IPlugin`, `Q_PLUGIN_METADATA`, `Q_INTERFACES(IPlugin)`.

```cpp
bool activate(PluginContext &context);
void deactivate();
```

`PluginContext` : `commands()`, `dialogParent()`, `workspaceRoot()`, `pluginDirectory()`, `log()`, `addViewer()`, `addPanel()`. `addViewer` : Editerako **prend la propriété** du provider.

## UI

Préférences → **Plugins** : liste, activer/désactiver, dossier utilisateur, recharger. Menu **Plugins** : commandes contribuées. `plugins/disabled` dans QSettings.

Détail : [adr/0020-plugin-system.md](adr/0020-plugin-system.md).
