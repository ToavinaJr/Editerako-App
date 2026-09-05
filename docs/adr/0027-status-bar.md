# ADR 0027 — Status bar modulaire

- **Statut :** accepté
- **Date :** 2026-09-05
- **Phase :** post-21 (§29)

## Contexte

Le cahier demande une status bar professionnelle : Ln, Col, Spaces/Tabs, Tab Size, Encoding, EOL, Language, Git, LSP, Problems — **chaque élément modulaire**. `EditorStatusWidget` existait déjà (Ln/Col, encoding, EOL, language, Git, LSP, debug, Problems) mais l’indentation manquait et les libellés étaient collés dans le widget.

## Décision

- **`StatusBarText`** : libellés testables, sans Widgets (`statusBarPositionLabel`, indent, tab size, problems).
- **`EditorStatusWidget`** : un `QLabel` par segment (objectName + tooltip). Segment vide → masqué. Problems reste un bouton (ouvre le panneau).
- Indentation lue depuis `AppSettings` (`insertSpaces`, `tabSize`). `applySettings()` après Préférences.
- Debug reste un segment extra (phase 19), après LSP.

## Conséquences

- Pas de picker encoding / EOL / indent au clic (Préférences).
- Tests : `test_StatusBarText`. Pas `MainWindow`.
