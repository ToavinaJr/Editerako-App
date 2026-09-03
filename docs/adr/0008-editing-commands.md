# ADR 0008 — Commandes d’édition

- **Statut :** accepté
- **Date :** 2026-09-03
- **Phase :** 8

## Contexte

L’éditeur avait Tab→espaces, Ctrl+Up/Down (swap de lignes) et le multi-curseur. Pas d’indent de sélection, commentaires, paires auto, matching, duplicate/delete/join/sort, occurrences. `highlightCurrentLine()` écrasait `setExtraSelections`.

## Décision

Features dans `src/editor/features/`, exposées via `CommandRegistry` / menu Edit. `CodeEditor` délègue.

| Type | Rôle |
|---|---|
| `IndentOps` / `IndentController` | indent/outdent, smart indent Enter, convert spaces/tabs |
| `CommentOps` / `CommentController` | toggle ligne / bloc ; tokens via `LanguageRegistry::commentTokens` |
| `AutoClosingPairs` | `()[]{}` `"` `'` ; skip closer ; wrap sélection |
| `BracketMatcher` | extra-selections fusionnées avec la ligne courante |
| `LineEditCommands` | duplicate, delete, select, join, sort, trim |
| `OccurrenceController` | Ctrl+D / Ctrl+Shift+L via extra cursors |
| `LineMovementController` | **inchangé** (`toPlainText().mid`) ; raccourcis désormais `edit.moveLineUp/Down` |

Tab / Shift+Tab restent dans `keyPressEvent` (pas de `QAction` Tab). Ctrl+Up/Down ne sont plus hardcodés dans `CodeEditor`.

Folding, completion, diagnostics : hors scope (Tree-sitter / LSP).

## Conséquences

- Commenter un `.txt` est un no-op (pas de tokens).
- Matching de brackets ignore chaînes/commentaires.
- Replace regex `$1` n’est pas concerné ici.
