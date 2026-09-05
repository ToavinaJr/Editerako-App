# ADR 0026 — Onglets : pin, preview, menu

- **Statut :** accepté
- **Date :** 2026-09-05
- **Phase :** post-21 (§28)

## Contexte

`EditorGroup` gérait déjà le réordre, le clic du milieu et un menu contextuel réduit. Le dirty `*` et le tooltip chemin vivaient dans `EditorManager`. Il manquait le pin, l’onglet preview (style VS Code) et Close to Right / Close Saved / Copy Path / Reveal.

## Décision

- **Pin** : drapeau sur le widget (`tabPinned`). Les pinned restent à gauche (`enforcePinOrder`). Clic du milieu et Close Others / Close to Right / Close Saved les ignorent. Fermer via ×, Close et Close All reste possible. Préfixe `📌 `.
- **Preview** (option `editor/previewTabs`, défaut true) : un seul par groupe, libellé `(nom)`, tooltip `Preview — path`. Clic explorateur → preview (remplace le précédent s’il n’est pas dirty). Double-clic, Open File, D&D, session, Problems, Search → permanent. Édition ou pin d’un preview → promote.
- **`TabOps`** : indices Close to Right / Close Saved / Close Others, sans Qt Widgets.
- Commandes palette : `file.closeToRight`, `file.closeSaved`, `file.pinTab`, `file.copyPath`, `file.reveal`.

## Conséquences

- Flags pin/preview voyagent avec le widget (split / Move Editor).
- Tests : `test_TabOps` (pur) et `test_EditorGroup` (offscreen). Pas `MainWindow`.
