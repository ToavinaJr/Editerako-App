# ADR 0022 — Viewers Markdown, SVG, CSV

- **Statut :** accepté
- **Date :** 2026-09-04
- **Phase :** post-21 (§45)

## Contexte

Le plan numéroté s’arrête à la phase 21. Le cahier demandait encore des viewers **Markdown, SVG, JSON, CSV**, en plus de PDF/images déjà livrés. `IFileViewerProvider` existe depuis les plugins (phase 20). `QPixmap` ne charge pas les SVG. Un viewer JSON par défaut remplacerait l’éditeur texte (highlighting Tree-sitter, LSP éventuellement).

## Décision

- **SVG** (`FileKind::Svg`) : `QSvgWidget` / Qt Svg, avant le branchement `image/`.
- **CSV** (`FileKind::Csv`) : table lecture seule (`QTableView`), parseur RFC 4180 minimal testé (`parseCsv`). Première ligne = en-tête.
- **Markdown** : reste **texte éditable**. Aperçu lecture seule via `file.markdownPreview` (`Ctrl+Shift+V`, `QTextBrowser::setMarkdown`), onglet distinct pour ne pas voler l’éditeur.
- **JSON** : reste **texte** (même raison que Markdown).
- Les providers plugins s’exécutent **avant** les viewers built-in.
- Dépendance Qt : `Svg` + `SvgWidgets` (archive aqt de base `qtsvg`, pas un `-m`).

## Conséquences

- Un `.csv` ne s’ouvre plus dans l’éditeur texte.
- Un `.svg` ne passe plus par `ImageViewer` (qui échouait).
- `README.md` reste éditable ; l’aperçu est une commande View.
