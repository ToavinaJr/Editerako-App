# ADR 0023 — Code folding Tree-sitter

- **Statut :** accepté
- **Date :** 2026-09-04
- **Phase :** post-21 (§26)

## Contexte

Le cahier demande un folding avec fold / unfold / fold all / unfold all, des indicateurs dans le gutter, et l’usage de Tree-sitter si possible. Phase 8 avait explicitement laissé le folding de côté.

## Décision

- Extraire les plages depuis l’arbre : nœuds **nommés** multi-lignes, racine exclue (`foldRangesFromTree`). Même start line → plage la plus large.
- `FoldingController` applique `QTextBlock::setVisible`. L’état plié est stocké sur le `QTextDocument` (vues split partagées).
- Gutter : chevrons entre breakpoints et diagnostics. Clic colonne fold → toggle ; clic gauche → breakpoint (inchangé).
- Commandes : `editor.toggleFold` (`Ctrl+Shift+[`), `editor.fold`, `editor.unfold`, `editor.foldAll` (`Ctrl+Alt+[`), `editor.unfoldAll` (`Ctrl+Alt+]`).
- Go To Line utilise `findBlockByNumber` (numéro de fichier) et déplie les plages qui couvrent la ligne.
- Sans highlighter (texte brut, fichier trop gros) : pas de folding.

## Conséquences

- Le folding est syntaxique, pas indentation-only.
- `}` de fin d’un bloc peut être masqué avec le corps.
