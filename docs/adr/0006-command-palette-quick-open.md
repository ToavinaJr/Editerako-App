# ADR 0006 — Command Palette et Quick Open

- **Statut :** accepté
- **Date :** 2026-09-03
- **Phase :** 6

## Contexte

Les actions vivaient dans `CommandRegistry` sans recherche. L’ouverture de fichier passait par l’explorateur ou un `QFileDialog`. Un parcours récursif `QDirIterator` *Subdirectories* entrerait dans `node_modules` / `build` et bloquerait le thread UI.

## Décision

- **`FuzzyMatcher`** (`EditerakoCore`) : score de sous-séquence, insensible à la casse, sans widgets. `parseFileLineQuery` extrait `fichier:ligne`.
- **`WorkspaceFileIndex`** (`EditerakoProject`) : walk récursif manuel qui saute chaque nom exclu (`Workspace::excludedNames`). Scan via `QtConcurrent::run` ; une génération ignore un résultat périmé. Plafond 50 000 fichiers.
- **`CommandPaletteDialog`** (`Ctrl+Shift+P`, `workbench.commandPalette`) : fuzzy sur le libellé et l’id, affiche le raccourci, déclenche la `QAction` si elle est enabled.
- **`QuickOpenDialog`** (`Ctrl+P`, `workbench.quickOpen`) : fichiers indexés + onglets déjà ouverts ; requête `nom:ligne` ouvre puis `EditorManager::goToLine`.
- UI partagée : `FuzzyPickerDialog`. Raccourcis uniquement via `KeybindingManager`.

Rebuild de l’index : `setRootPath`, `reloadExplorer` (exclusions), création fichier/dossier, `FileWatcher::rootContentsChanged` (debounce 250 ms). Sans workspace, Quick Open ne liste que les fichiers ouverts.

## Conséquences

- `EditerakoProject` lie `Qt6::Concurrent`.
- L’index n’est pas un snapshot synchrone : Quick Open affiche le dernier résultat et se rafraîchit sur `indexUpdated`.
- Overlay language-specific reste hors scope.
