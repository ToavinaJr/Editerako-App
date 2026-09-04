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

`files/hotExit` (défaut true) : quitter sans dialogue Save All ; les unsaved (hors secrets) sont restaurés au prochain lancement. Les backups vivent sous AppData (`backups/`), pas dans le workspace. [adr/0017-recovery-hot-exit.md](adr/0017-recovery-hot-exit.md).

Raccourcis : overrides user sous `keybindings/<commandId>` (QSettings). Les conflits sont refusés. Défauts : `workbench.commandPalette` = `Ctrl+Shift+P`, `workbench.quickOpen` = `Ctrl+P`, `workbench.search` = `Ctrl+Shift+F`, `workbench.splitEditorRight` = `Ctrl+\`, `workbench.splitEditorDown` = `Ctrl+Shift+\`, `edit.toggleLineComment` = `Ctrl+/`, `edit.moveLineUp/Down` = `Ctrl+Up/Down`, `edit.selectNextOccurrence` = `Ctrl+D`. Détail : [adr/0005-settings-keybindings.md](adr/0005-settings-keybindings.md), [adr/0008-editing-commands.md](adr/0008-editing-commands.md), [adr/0018-editor-groups.md](adr/0018-editor-groups.md).
