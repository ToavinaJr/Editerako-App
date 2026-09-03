# ADR 0012 — Problems Panel

- **Statut :** accepté
- **Date :** 2026-09-03
- **Phase :** 12

## Contexte

Les diagnostics LSP étaient visibles uniquement dans l’éditeur (underline + gutter). Le cahier des charges demande un panneau **Problems / Errors / Warnings**, avec double-clic vers fichier / ligne / colonne. Le panneau inférieur générique (Output, Debug Console) reste hors scope ; le terminal actuel ne doit pas casser.

## Décision

- `ProblemModel` / `ProblemItem` (éditeur) : store par fichier, filtres All / Errors / Warnings, **sans types LSP**.
- `ProblemsPanel` : arbre groupé par fichier, filtres, activation double-clic / Enter.
- `BottomPanel` : onglets `Problems` + `Terminal`. `TerminalPanel` ne gère plus sa visibilité externe (`showRequested` / `hideRequested`) pour rester un contenu d’onglet.
- `LspSession` émet `problemsChanged(path, items)` à chaque `publishDiagnostics` et vide le fichier au `didClose`.
- `Ctrl+Shift+M` (`workbench.problems`) affiche / masque Problems. `Ctrl+J` conserve le quirk du premier toggle terminal.

## Conséquences

- Un diagnostic peut apparaître dans Problems même si l’éditeur n’est pas l’onglet actif (clangd sur un header).
- Output / Debug Console : phases ultérieures.
- Compteurs : titre d’onglet `Problems (n)` + bouton status bar.
