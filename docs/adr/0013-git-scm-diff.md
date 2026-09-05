# ADR 0013 — Git CLI, Source Control et Diff

- **Statut :** accepté
- **Date :** 2026-09-04
- **Phase :** 13

## Contexte

Le cahier des charges demande un SCM (`src/scm/`), un premier provider Git, un panneau Source Control, des décorations dans l’explorateur, et un viewer de diff générique (Git, compare-with-disk). libgit2 n’est pas justifié : le CLI Git est déjà l’outil de l’utilisateur.

## Décision

- `ISourceControlProvider` : `refresh`, `stage`, `unstage`, `discard`, `commit`, `requestDiff`.
- `GitCliProvider` exécute `git` via `QProcess` **hors thread UI** (`QtConcurrent` + `GitProcess::run`). File d’attente + jeton de génération pour ignorer les résultats périmés.
- Parser `git status --porcelain=v1 -z` (`GitParsers`) ; chemins rendus absolus via `rev-parse --show-toplevel`. Branche : detached (`HEAD (no branch)`), dépôt vide (`No commits yet on`), ahead/behind (`[ahead N, behind M]`) affichés dans le panneau et la status bar.
- UI dans le `BottomPanel` : onglets **Source Control** et **Diff** (unified, coloré). Compare-with-disk utilise `TextDiff`.
- Décorations explorateur : badges `M`/`A`/`D`/`U`/… via `FileExplorer::setPathBadges` (pas de types SCM dans `project/`).
- Git absent ou dossier hors dépôt : statut vide, pas de crash.

## Conséquences

- `EditerakoScm` dépend de `EditerakoCore` + `Qt6::Concurrent`. Aucun Widget dans `src/scm/`.
- Discard untracked : `git clean -f`. Restore worktree pour le reste.
- Diff side-by-side : non livré ; le unified suffit pour Git et compare-with-disk.
- Push / pull / merge / blame : hors phase 13.
