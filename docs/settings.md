# Settings

Priorité de lecture :

```
défaut  <  user (QSettings org/app Editerako)  <  {workspace}/.editerako/settings.json
```

L’UI **Préférences** (View → Preferences, `Ctrl+,`) écrit le profil user. Pour un projet, créer `{workspace}/.editerako/settings.json` :

```json
{
  "theme": "light",
  "editor": {
    "fontFamily": "Consolas",
    "fontSize": 13,
    "tabSize": 4,
    "insertSpaces": false,
    "wordWrap": false,
    "lineNumbers": true
  },
  "files": {
    "autoSave": false,
    "autoSaveDelayMs": 1000,
    "largeFileWarnBytes": 5242880,
    "largeFileDisableSyntaxBytes": 20971520
  },
  "workspace": {
    "excludedFolders": [".git", "node_modules", "build"]
  },
  "terminal": {
    "shell": ""
  },
  "ai": {
    "provider": "gemini",
    "model": "gemini-2.0-flash-001",
    "endpoint": ""
  }
}
```

Les clés API ne vont **pas** dans ce fichier. Utiliser `.env` (`GEMINI_API_KEY`).

Raccourcis : overrides user sous `keybindings/<commandId>` (QSettings). Les conflits sont refusés. Détail : [adr/0005-settings-keybindings.md](adr/0005-settings-keybindings.md).
