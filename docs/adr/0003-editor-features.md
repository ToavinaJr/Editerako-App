# ADR 0003 — Extraire les features de l’éditeur

- **Statut :** accepté
- **Date :** 2026-09-03
- **Phase :** 3

## Contexte

`CodeEditor` mélangeait gutter, highlight de ligne, multi-curseurs et swap de lignes. `EditorManager` mélangeait onglets, I/O, style et synchronisation du highlighter. Les features existaient déjà ; il ne s’agissait pas d’en inventer (folding, LSP, autocomplete).

## Décision

`CodeEditor` reste le widget central. Les responsabilités déjà présentes sont extraites dans `src/editor/features/` :

| Type | Rôle |
|---|---|
| `LineNumberArea` | Widget gutter ; le paint reste délégué à `CodeEditor` |
| `CurrentLineHighlighter` | Extra-sélection de la ligne courante |
| `MultiCursorController` | Cursors extra, Alt+clic, insert/delete, paint des carets |
| `LineMovementController` | Ctrl+Up/Down — **même algorithme** `toPlainText().mid` |

`EditorManager` reste l’orchestrateur d’onglets et de dialogs. Extraire uniquement :

| Type | Rôle |
|---|---|
| `EditorIo` | Lecture texte (`QFile` + `QTextStream` Text) |
| `EditorStyle` | Police, tab, wrap, numéros depuis `AppSettings` |
| `HighlighterSync` | Attacher / retirer `SyntaxHighlighter` ; `shouldHighlight` testable |

**Non créés :** AutoIndent, BracketMatcher, AutoClosingPairs, Folding, Completion, Diagnostics, EditorGroups. L’écriture reste `writeTextAtomically`. Le couplage Viewers → Editor (PDF/images dans le même `QTabWidget`) n’est pas traité ici ; il relève des groupes d’éditeurs (Phase 18).

## Conséquences

- `highlightCurrentLine()` continue d’écraser `setExtraSelections` (pas de diagnostics encore).
- Ctrl+Up/Down est géré **avant** le multi-curseur, y compris s’il y a des extra cursors.
- Includes : `"editor/features/MultiCursorController.h"`.
