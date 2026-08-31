# Architecture

Editerako est **un seul exécutable Qt Widgets**. Les dossiers sous `src/` sont des modules logiques, pas des bibliothèques séparées. `MainWindow` orchestre ; la logique métier vit dans les modules.

```
src/main.cpp
    QApplication, thème, MainWindow
        │
        ▼
src/app/MainWindow          orchestration UI
        ├── editor/         onglets texte, save/close
        ├── viewers/        PDF, images, fileKindForPath
        ├── project/        workspace, arbre, QFileSystemWatcher
        ├── terminal/       panneau d'onglets + QProcess
        ├── ai/             chat Gemini + SQLite
        └── core/           settings, session, commandes, thème
```

Includes : `#include "module/Fichier.h"` avec `src/` comme seul chemin d’include applicatif.

## Modules

| Dossier | Responsabilité |
|---|---|
| `app/` | Fenêtre principale, menus, drag-and-drop, dialogue d’accueil, câblage des signaux |
| `core/` | `AppSettings`, `SessionStore`, `CommandRegistry`, `ThemeManager`, `DropPaths`, catégories de log |
| `editor/` | `CodeEditor`, `EditorDocument`, `EditorManager`, find / go-to-line |
| `syntax/` | `LanguageRegistry` (C++ / HTML), `TreeSitterDocument`, `SyntaxHighlighter` |
| `project/` | `Workspace` (racine, exclusions, création fichier/dossier), `FileExplorer`, `FileWatcher` |
| `terminal/` | `Terminal` (widget), `TerminalProcess` (`QProcess`), `TerminalPanel` (onglets, + / ×) |
| `viewers/` | `fileKindForPath`, `ViewerManager`, `PdfViewer`, `ImageViewer` |
| `ai/` | `AiProvider` / `GeminiProvider`, `ChatWidget`, `ChatRepository`, `ContextBuilder` |
| `ui/` | Helpers de layout / `QInputDialog` (pas de widgets métier) |

Tree-sitter (runtime + grammaires C++ et HTML) est vendored dans `tree-sitter/` et compilé via `cmake/TreeSitter.cmake`. Seul `tree_sitter/api.h` est PUBLIC pour l’application.

## Flux importants

**Ouvrir un fichier.** `MainWindow::openFileInEditor` → `ViewerManager::open` → `fileKindForPath` : texte (`EditorManager`), PDF, image, ou page « non supporté ».

**PDF.** Ne pas appeler `QPdfView::setDocument` avant `QPdfDocument::Status::Ready` (crash sinon). L’onglet est ajouté d’abord, le document ensuite.

**Session.** `SessionStore` lit/écrit `QSettings` (org/app `Editerako`). Restaure workspace, onglets existants, fichier actif, géométrie. Les *untitled* ne sont pas restaurés. Le dialogue d’accueil n’apparaît que s’il n’y a pas de session valide.

**Disque.** `FileWatcher` surveille la racine (debounce 250 ms) et les fichiers ouverts. `EditorManager::aboutToSave` appelle `ignoreNextChange` pour ne pas recharger nos propres sauvegardes.

**Terminal.** Cwd = dossier du fichier texte actif, sinon le workspace. Fermer le dernier onglet masque le panneau. `Ctrl+J` bascule la visibilité (après setup le panneau est masqué avec le flag « visible » : le premier `Ctrl+J` ne fait qu’aligner l’état).

**Chat.** `GEMINI_API_KEY` (`.env` chargé au démarrage). Historique SQLite : `{projet}/.editerako/chat_history.db` (non versionné). Modèle : `gemini-2.0-flash-001`. `AiProvider::create` est le point d’extension pour un autre backend.

## Ce que MainWindow ne doit plus contenir

Pas de logique d’onglets terminal, pas d’I/O fichier brut, pas de classification MIME. Déléguer à `TerminalPanel`, `Workspace`, `fileKindForPath`.
