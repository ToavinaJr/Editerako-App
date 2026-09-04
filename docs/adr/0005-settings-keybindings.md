# ADR 0005 — Settings et raccourcis

- **Statut :** accepté
- **Date :** 2026-09-03
- **Phase :** 5

## Contexte

Les préférences existaient dans `AppSettings` (QSettings) sans UI. Les raccourcis étaient hardcodés dans `MainWindow`. Pas de overlay workspace, pas de détection de conflits.

## Décision

- **Priorité de lecture :** défaut < user (`QSettings`) < `{workspace}/.editerako/settings.json`. Les setters de l’UI écrivent le **profil user**. Overlay language-specific réservé, pas implémenté.
- **UI** `SettingsDialog` (catégories General, Editor, Files, Workspace, Terminal, Appearance, Keyboard, AI). Pas de champ clé API ; sign-in compte par défaut, clés API dans `.env`.
- **`KeybindingModel`** : table des défauts unique. Overrides sous `keybindings/<commandId>`. **`KeybindingManager`** applique les séquences aux `QAction` du `CommandRegistry`. Conflit = refus.
- Raccourcis **inchangés** par défaut (New/Open/Save/SaveAs, Ctrl+W, Find, Ctrl+G, Ctrl+J). `preferences.open` = Ctrl+,.
- `insert spaces` **désactivé** par défaut (Tab insère encore `\t`). Auto-save **off** par défaut ; ignore les untitled / read-only.

## Conséquences

- `MainWindow` n’appelle plus `setShortcut` à la main.
- Changer le thème / la police / les exclusions recharge QSS, style éditeur, arbre.
- Les tests de settings passent un `QSettings` Ini temporaire, pas le profil utilisateur.
