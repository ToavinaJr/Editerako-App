# ADR 0002 — MainWindow composition root

- **Statut :** accepté
- **Date :** 2026-09-03
- **Phase :** 2

## Contexte

`MainWindow` (~630 lignes) mélangeait câblage UI, fan-out workspace, persistance de session et politique de reload disque. `EditorManager`, `ViewerManager`, `TerminalPanel` et `ChatWidget` existaient déjà.

## Décision

`MainWindow` reste le **composition root Qt** (menus, dialogs, `closeEvent`, D&D). Extraire uniquement les responsabilités qui ont une vraie cohésion :

| Type | Rôle | Pourquoi |
|---|---|---|
| `WorkspaceController` (`EditerakoProject`) | Facade `Workspace` + `FileExplorer` + `FileWatcher` | Un seul endroit pour ouvrir un projet et synchroniser l’arbre / le watcher |
| `SessionController` (`EditerakoCore`) | Garde restore, save no-op pendant restore, filtrage des fichiers existants | `SessionStore` reste l’I/O ; le contrôleur porte la politique |
| `diskChangeAction()` (`EditerakoCore`) | Décision reload / close / prompt | Testable sans widgets |

**Non créés :** `ApplicationController` (ce serait un second God Object), `EditorController` (`EditorManager` suffit), `UiStateController` (titre, commandes, collapse explorer restent de la UI).

Les collaborateurs (éditeur, terminal, chat) s’abonnent à `WorkspaceController::rootPathChanged`. Ils ne sont pas injectés dans le contrôleur projet.

## Conséquences

- Ajouter un collaborateur workspace = un `connect` dans `MainWindow::connectWorkspaceCollaborators`.
- Les `QMessageBox` restent dans `MainWindow`.
- Couplage Viewers → Editor inchangé (Phase 3).
