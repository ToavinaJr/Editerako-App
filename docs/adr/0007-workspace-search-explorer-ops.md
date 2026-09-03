# ADR 0007 — Recherche workspace et opérations explorateur

- **Statut :** accepté
- **Date :** 2026-09-03
- **Phase :** 7

## Contexte

L’explorateur avait un `CustomContextMenu` sans handler. Pas de recherche multi-fichiers. Les créations de fichiers acceptaient n’importe quel nom, y compris `../`.

## Décision

- **`WorkspacePath`** : `isSafeRelativePath` refuse `..` et les chemins absolus ; `isInsideWorkspace` vérifie le préfixe *et* `canonicalFilePath()` quand le fichier existe (symlink).
- **`WorkspaceOps`** : rename / duplicate / copy / move / delete. Delete UI = corbeille (`QFile::moveToTrash`). La racine du workspace n’est jamais supprimée ni renommée.
- **`GitIgnore`** : sous-ensemble (commentaires, `!`, `*`, `**`, `/` ancré, trailing `/`). `.git` toujours ignoré. Globs include/exclude via `globMatches`.
- **`WorkspaceSearch`** : compile query (texte / regex / case / whole word), `findInText` / `replaceInText` testables, scan `QtConcurrent` + annulation coopérative. Respecte exclusions Editerako, `.gitignore`, fichiers binaires et gros fichiers (`largeFileWarnBytes`).
- **UI** : `WorkspaceSearchDialog` non modal (`Ctrl+Shift+F`, `workbench.search`). Résultats groupés par fichier, preview, replace / replace all. Si le fichier est ouvert, le replace passe par l’éditeur (un undo).
- **Explorateur** : menu New/Rename/Delete/Duplicate/Copy/Cut/Paste/chemins/Reveal/Terminal/Refresh/Collapse All.

`nested/dir` reste autorisé (pas de `..`).

## Conséquences

- Parser `.gitignore` incomplet vs git (pas de nested ignore files, pas de `**` avancé complet).
- Replace regex n’étend pas `$1`.
- Delete tests utilisent `Permanent` ; l’UI utilise la corbeille.
