# ADR 0025 — Recent workspaces

- **Statut :** accepté
- **Date :** 2026-09-04
- **Phase :** post-21 (§42)

## Contexte

Le cahier demande **File > Open Recent** et une **welcome page** des derniers projets, une liste plafonnée, et le retrait des chemins disparus. L’accueil était un `QMessageBox` sans historique.

## Décision

- `RecentWorkspaces` (Core, `QSettings` `recent/workspaces`) : 10 entrées, plus récent en tête, déduplication, `prune()` des dossiers inexistants. Un workspace n’est mémorisé que s’il existe (Open Folder, drop, restore session, choix welcome). Cancel / fallback Documents ne pollue pas la liste.
- Menu **File > Open Recent** reconstruit à `aboutToShow`. Entrée manquante → warning + retrait. **Clear Recently Opened** vide la liste.
- `WelcomeDialog` remplace le QMessageBox : liste cliquable, Open Folder / Open File / Cancel, menu contextuel « Remove from List ».

## Conséquences

- Pas de commande palette par entrée récente (sous-menu dynamique).
- Tests : round-trip / prune / plafond, pas `MainWindow`.
