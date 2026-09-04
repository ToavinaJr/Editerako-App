# Settings

Priorité de lecture :

```
défaut  <  user (QSettings org/app Editerako)  <  {workspace}/.editerako/settings.json
```

L’UI **Préférences** (View → Preferences, `Ctrl+,`) écrit le profil user. Pour un projet, créer `{workspace}/.editerako/settings.json` :

```json
{
  "theme": "light",
  "ui": {
    "language": "fr"
  },
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
    "hotExit": true,
    "largeFileWarnBytes": 5242880,
    "largeFileDisableSyntaxBytes": 20971520
  },
  "workspace": {
    "excludedFolders": [".git", "node_modules", "build"]
  },
  "terminal": {
    "shell": "",
    "usePty": false
  },
  "ai": {
    "provider": "chatgpt",
    "model": "",
    "endpoint": ""
  }
}
```

Les clés API ne vont **pas** dans ce fichier. Le chat par défaut est un **sign-in** (ChatGPT / Claude / compte Google / Copilot). Pour un backend API, utiliser `.env` (`GEMINI_API_KEY`, `OPENAI_API_KEY`, `ANTHROPIC_API_KEY`).

`ui/language` (vide = locale système, `en`, `fr`) : catalogues Linguist. Un changement dans Préférences → Apparence s’applique au prochain lancement. [adr/0024-i18n.md](adr/0024-i18n.md).

`files/hotExit` (défaut true) : quitter sans dialogue Save All ; les unsaved (hors secrets) sont restaurés au prochain lancement. Les backups vivent sous AppData (`backups/`), pas dans le workspace. [adr/0017-recovery-hot-exit.md](adr/0017-recovery-hot-exit.md).

Raccourcis : overrides user sous `keybindings/<commandId>` (QSettings). Les conflits sont refusés. Défauts : `workbench.commandPalette` = `Ctrl+Shift+P`, `workbench.quickOpen` = `Ctrl+P`, `workbench.search` = `Ctrl+Shift+F`, `workbench.splitEditorRight` = `Ctrl+\`, `workbench.splitEditorDown` = `Ctrl+Shift+\`, `file.markdownPreview` = `Ctrl+Shift+V`, `editor.toggleFold` = `Ctrl+Shift+[`, `editor.foldAll` = `Ctrl+Alt+[`, `editor.unfoldAll` = `Ctrl+Alt+]`, `edit.toggleLineComment` = `Ctrl+/`, `edit.moveLineUp/Down` = `Ctrl+Up/Down`, `edit.selectNextOccurrence` = `Ctrl+D`. Détail : [adr/0005-settings-keybindings.md](adr/0005-settings-keybindings.md), [adr/0008-editing-commands.md](adr/0008-editing-commands.md), [adr/0018-editor-groups.md](adr/0018-editor-groups.md), [adr/0022-file-viewers.md](adr/0022-file-viewers.md), [adr/0023-code-folding.md](adr/0023-code-folding.md).

Plugins locaux : dossiers `%AppData%/Editerako/plugins` et `{workspace}/.editerako/plugins`. Désactivation : `plugins/disabled` (QSettings). UI Préférences → Plugins. [plugins.md](plugins.md), [adr/0020-plugin-system.md](adr/0020-plugin-system.md).
