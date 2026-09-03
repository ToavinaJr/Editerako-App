# Baseline de refactoring — Phase 0

**Date :** 2026-09-03  
**Version du projet :** 0.1.0  
**Plateforme d’audit :** Windows 10, MinGW-w64 GCC 13.1.0, CMake 3.30.5, Qt 6.11.2 (`mingw_64`)  
**Statut :** audit uniquement. Aucune refonte de code.

Ce document fige l’état du dépôt **avant** les phases 1–21. Il sert de référence pour les migrations suivantes : ne pas supprimer une feature listée comme fonctionnelle.

---

## 1. Architecture actuelle

Editerako est **un seul exécutable Qt Widgets**. Les dossiers sous `src/` sont des modules **logiques**, pas des bibliothèques CMake.

```
src/main.cpp
    QApplication, .env, ThemeManager, MainWindow
        │
        ▼
src/app/MainWindow          composition + orchestration UI (trop large)
        ├── editor/         onglets texte, save/close, find, go-to-line
        ├── viewers/        PDF, images, fileKindForPath
        ├── project/        workspace, arbre, QFileSystemWatcher
        ├── terminal/       panneau d’onglets + QProcess
        ├── ai/             chat Gemini + SQLite
        ├── syntax/         Tree-sitter C++ / HTML
        ├── ui/             helpers de layout / QInputDialog
        └── core/           settings, session, commandes, thème, I/O atomique
```

**Direction de dépendances observée (implicite, non imposée par CMake) :**

| Module | Dépend de | Ne devrait pas dépendre de |
|---|---|---|
| `core/` | Qt Core/Widgets (`QAction`, `QSettings`) | `app/`, widgets métier |
| `syntax/` | Tree-sitter, `core/Logging` | `app/`, `editor/` (sauf `QTextDocument`) |
| `editor/` | `core/`, `syntax/` | `app/`, `ai/`, `terminal/` |
| `project/` | `core/AppSettings` | `app/`, `editor/` |
| `viewers/` | `editor/EditorManager`, Qt PDF | `app/` |
| `terminal/` | Qt Concurrent, `QProcess` | `app/`, `editor/` |
| `ai/` | Qt Network/Sql, `core/` | `app/` (sauf via `MainWindow`) |
| `app/` | **tous les modules** | — (composition root) |

**Pas de dépendance circulaire C++ détectée.** Le couplage est en étoile autour de `MainWindow`.

`core/` n’inclut pas `MainWindow`, mais `CommandRegistry` dépend de `QAction`/`QWidget` : Core n’est pas encore « headless ».

---

## 2. CMake et build

### 2.1 Cibles actuelles

| Cible | Type | Rôle |
|---|---|---|
| `Editerako` | `qt_add_executable` | Application unique, toutes les sources listées explicitement |
| `tree_sitter` | `STATIC` | Runtime + grammaires C++ et HTML vendored |
| `test_*` (12) | `qt_add_executable` | Tests Qt Test ; **recompilent** les `.cpp` métier au lieu de lier une lib |

Pas de `file(GLOB)`. Pas d’`include_directories()` global. Includes applicatifs : `src/` en `PRIVATE`.

### 2.2 Points forts

- CMake ≥ 3.21, C++20, Ninja, presets `debug` / `release`
- `target_include_directories` / `target_link_libraries` / `target_compile_options`
- Warnings `-Wall -Wextra -Wpedantic` (GCC) ou `/W4` (MSVC) **uniquement** sur app/tests
- Tree-sitter compilé avec `-w` / `/W0`
- `CMAKE_RUNTIME_OUTPUT_DIRECTORY` = racine du build (scripts cohérents)
- `qt_generate_deploy_app_script` pour l’install
- Workaround macOS AGL (`cmake/Apple.cmake`)

### 2.3 Limites CMake

- Un seul target applicatif : les tests dupliquent la compilation des sources
- Pas de `target_compile_features` explicite (standard global `CMAKE_CXX_STANDARD 20`)
- Pas de presets sanitizers (`EDITERAKO_ENABLE_ASAN` / `UBSAN` absents)
- Pas de warnings-as-errors, même en CI
- `docs/build.md` et `docs/testing.md` indiquent encore **7** tests (il y en a **12**)

---

## 3. Features existantes (fonctionnelles)

Ne pas casser :

| Domaine | Comportement observé |
|---|---|
| Éditeur | `QPlainTextEdit`, numéros de ligne, highlight ligne courante, multi-curseurs (Ctrl+clic), swap de lignes (Ctrl+Up/Down) |
| Fichiers | New / Open / Save / Save As / Save All, dirty `*`, untitled, Save As si untitled |
| Onglets | Closables, réordonnables (`setMovable`), close current/others/all, tooltip chemin |
| Find / Replace | Recherche, regex, case sensitive, replace, replace all (dialog modal) |
| Go to line | Ctrl+G |
| Workspace | Open folder, exclusions par nom (`.git`, `node_modules`, `build`, …) |
| Explorer | Arbre lazy, icônes emoji, New File / New Folder (boutons + menu), reveal après création |
| Watcher | Racine debounce 250 ms, fichiers ouverts, `ignoreNextChange` au save |
| Session | Workspace, fichiers ouverts existants, fichier actif, géométrie, windowState |
| Drag & drop | Fichiers → ouvrir, dossiers → workspace |
| Syntaxe | Tree-sitter **C++** (y compris `.c`/`.h`) et **HTML**, queries SCM embarquées |
| Gros fichiers | Seuil warning 5 Mo, désactivation syntaxe 20 Mo |
| Sauvegarde | `QSaveFile` via `writeTextAtomically` |
| Reload disque | Prompt si dirty, reload si clean, warning si fichier supprimé |
| PDF | `QPdfView` seulement si `Status::Ready` |
| Images | Affichage scaled, fond sombre |
| Terminal | Multi-onglets, `QProcess` (`cmd.exe /c` / `$SHELL -c`), historique, autocomplete, `cd`/`pwd`/`clear` locaux |
| IA | Gemini via `QNetworkAccessManager`, historique SQLite `.editerako/chat_history.db`, contexte fichier actif tronqué |
| Thème | QSS dark/light embarqués, `AppSettings::themeId()` |
| Commandes | `CommandRegistry` pour File / Find / Toggle Terminal |
| Logs | `QLoggingCategory` par module (`lcCore`, `lcEditor`, …) |
| Accueil | Dialogue Open Folder / Open File / Cancel si session invalide |

---

## 4. Features partielles

| Feature | État | Écart |
|---|---|---|
| `LanguageRegistry` | IDs pour C, CMake, CSS, JS, TS, TSX, JSON, MD, Python, Shell, SQL, YAML | **Pas de grammaire Tree-sitter** sauf C++/HTML. C réutilise `tree_sitter_cpp`. Tests documentent explicitement `tsLanguage(Python) == nullptr` |
| `AppSettings` | Lecture thème, police, tab size, wrap, line numbers, exclusions, provider IA, seuils gros fichiers | Presque **aucun setter** (sauf `setThemeId`). **Pas d’UI Settings**. Police par défaut `Consolas` (Windows). Pas d’insert-spaces, auto-save, shell, modèle IA |
| `CommandRegistry` | IDs + `QAction` + raccourcis | Raccourcis encore **hardcodés** dans `MainWindow`. Pas de persistance, pas de conflits, pas de palette |
| `EditorDocument` | path, displayName, dirty, untitled | Pas d’encoding, BOM, EOL, language, read-only, cursor, scroll, version |
| Multi-curseurs | Insert / backspace / delete | Pas de sélection multi, pas de flèches coordonnées, `highlightCurrentLine()` **écrase** les extra selections |
| File Explorer | `Qt::CustomContextMenu` activé | **Aucun handler** de menu contextuel |
| Chat IA | Envoi + historique SQLite + markdown assistant | `saveChatHistory()` **vide**. Pas de cancel UI, retry, New Chat, copy code, streaming. Provider/modèle hardcodés (`gemini-2.0-flash-001`). HTML user cassé (style non fermé) |
| `AiProvider` | Factory + `send` / `responseReady` / `errorOccurred` | Pas de `cancel`, `capabilities`, `models`. Tout provider inconnu → Gemini |
| Terminal | Couleurs manuelles, clear, cwd | Pas d’ANSI, pas de PTY/ConPTY, pas de resize, pas de profils. `waitForFinished` au shutdown (UI) |
| Status bar | Messages transitoires (save, selected, reload) | Pas de Ln/Col, encoding, EOL, language, Git, LSP |
| Session | Workspace + onglets + géométrie | Pas de `sessionVersion`, cursor/scroll, groupes, panneaux, sidebar sizes. Untitled non restaurés (volontaire) |
| Thème | QSS global | Couleurs syntaxe / gutter / current line **hardcodées** dans `SyntaxHighlighter` / `CodeEditor` |
| i18n | Beaucoup de `tr()` | `FindReplaceDialog` et plusieurs chaînes terminal **sans** `tr()`. Pas de `.ts` / `.qm`. UI `.ui` en anglais + typo « Show lignes » |
| Tests | 12 cibles unitaires solides | Docs obsolètes (7 tests). Pas d’intégration, pas d’UI widget hors offscreen CommandRegistry/TreeSitter |

### Actions UI déclarées mais non branchées

Dans `MainWindow.ui`, jamais connectées :

- `actionDebug`, `actionOptimize`, `actionTranslate`, `actionDocumentation`, `actionGenerate_Code`, `actionHome`, `actionNew_File`

Ce sont des **stubs de Designer**, pas des features.

---

## 5. Features manquantes (phases 5–21)

| Zone | Absent |
|---|---|
| Édition | indent/outdent, auto-close, bracket match, comments, duplicate/delete/join/sort lines, folding, occurrences, trim whitespace |
| Navigation | Command Palette, Quick Open, Search workspace, F12, references, rename, symbols |
| LSP / clangd | module `src/lsp/` entier |
| Diagnostics | underline, gutter, Problems panel |
| Git / SCM | `src/scm/`, decorations explorer, Diff viewer |
| Tasks / CMake | `src/tasks/`, `.editerako/tasks.json`, presets CMake |
| Debugger | DAP |
| Layout | EditorArea / EditorGroup / split, bottom panel générique, status bar modulaire |
| Settings | UI, couches default/user/workspace/language, `.editerako/settings.json` |
| Recovery | BackupService / Hot Exit |
| Plugins | IPlugin / PluginManager |
| Viewers | Markdown, SVG, JSON, CSV, `IFileViewerProvider` |
| Packaging | Windows installer/portable, Linux AppImage (macOS DMG CI seulement) |
| Qualité | `.clang-format`, `.clang-tidy`, sanitizers, CI multi-OS + CTest |

---

## 6. Tests existants

Exécutés le 2026-09-03 : **12/12 OK** Debug et Release.

| Cible | Couverture | Plateforme test |
|---|---|---|
| `test_LanguageRegistry` | extensions, displayName, pointeurs TS, queries vides pour langs futurs | GUI-less |
| `test_Workspace` | root, `containsPath`, exclusions, create file/folder, list | GUI-less + `QTemporaryDir` |
| `test_DropPaths` | `QMimeData` URLs | GUI-less |
| `test_SessionStore` | round-trip Ini temporaire | GUI-less |
| `test_ContextBuilder` | prompt, troncature 8000, fenêtre historique | GUI-less |
| `test_FileKind` | text / PDF / image / vide | GUI-less |
| `test_CommandRegistry` | create, doublons, `setEnabled` | `QT_QPA_PLATFORM=offscreen` |
| `test_AtomicFile` | write, overwrite, Unicode, path vide / dossier | GUI-less |
| `test_CommandHistory` | empty, doublons, navigation | GUI-less |
| `test_CommandCompleter` | préfixe, historique, args git, paths cwd | GUI-less |
| `test_TreeSitterDocument` | parse C++, edit, unicode, HTML comment | offscreen |
| `test_HighlightQuery` | captures C++/HTML, predicates | GUI-less + `EDITERAKO_QUERY_DIR` |

**Non testé :** encoding/EOL, recovery, session migration, MainWindow, FileExplorer ops, EditorManager save/reload, Gemini, terminal process, PDF, path traversal, LSP, Git.

---

## 7. CI, scripts, ressources

### CI (`.github/workflows/macos-build.yml`)

- **macOS uniquement** (push/PR `master`)
- Qt 6.9.2 + qtpdf via aqtinstall, CMake Ninja Release
- Package DMG (`create-dmg`), upload artifact
- **N’exécute pas CTest**
- Pas de Windows, Ubuntu, clang-format, clang-tidy, sanitizers, CodeQL

### Scripts

Windows (`scripts/*.ps1`) et Linux (`scripts/*.sh`) : configure / build / run / test / clean. Détection Qt via `CMAKE_PREFIX_PATH`, `QTDIR`, `qtpaths`/`qmake`, ou `C:\Qt\<ver>\mingw_64`. Pas de script macOS dédié (CI inline).

### Ressources

| Chemin | Contenu |
|---|---|
| `resources/themes/dark.qss`, `light.qss` | Thèmes QSS |
| `resources/syntax/cpp/highlights.scm` | Query Tree-sitter C++ |
| `resources/syntax/html/highlights.scm` | Query Tree-sitter HTML |
| `tree-sitter/` | Runtime + `tree-sitter-cpp` + `tree-sitter-html` |
| `.env.example` | `GEMINI_API_KEY=` (`.env` gitignored) |

---

## 8. Classes volumineuses

Seuil indicatif ~250 lignes. Mesure : lignes physiques (Get-Content).

| Fichier | Lignes | Verdict |
|---|---|---|
| `app/MainWindow.cpp` | **629** | God-orchestrator : session, FS, welcome, D&D, watcher, chat context, commandes |
| `editor/EditorManager.cpp` | **475** | Onglets + I/O + dialogs + highlighter + viewers tabs |
| `editor/CodeEditor.cpp` | **313** | Gutter + current line + multi-cursor + move line |
| `terminal/CommandDiscovery.cpp` | **311** | Builtins + cache + scan PATH + parse `--help` |
| `project/FileExplorer.cpp` | 246 | OK mais menu contextuel à venir |
| `terminal/Terminal.cpp` | 241 | OK, messages non i18n |
| `terminal/TerminalInput.cpp` | 212 | Deux classes (`AutoCompletePopup` + `TerminalTextEdit`) |
| `syntax/HighlightQuery.cpp` | 198 | Dense mais cohésif (predicates) |
| `syntax/TreeSitterDocument.cpp` | 182 | Parse incrémental sur le thread UI |
| `ai/ChatWidget.cpp` | 181 | UI + HTML + provider + repo |

`LineNumberArea` est déjà extraite dans `CodeEditor.h` (classe imbriquée). Pas de `editor/features/` encore.

---

## 9. Dette technique (priorisée)

### 9.1 Architecture / SOLID

- `MainWindow` porte session, filesystem, welcome, D&D, watches, chat, command states.
- `EditorManager` mélange modèle d’onglets, I/O, `QMessageBox`, style, highlighter.
- `EditorDocument` est un **adapter** 1:1 sur `CodeEditor`, pas un document partageable multi-vues.
- `ViewerManager` dépend de `EditorManager` pour les tabs (PDF/images dans le même `QTabWidget`).
- Tests recopient les `.cpp` → CMake non modulaire.

### 9.2 Thread UI

| Opération | Où | Risque |
|---|---|---|
| Lecture fichier entier (`QTextStream::readAll`) | `EditorManager::openTextFile` / `reloadFromDisk` | Freeze gros fichiers (mitigé par warning 5 Mo) |
| Parse Tree-sitter + snapshot `QStringList` | `TreeSitterDocument::onContentsChange` | Freeze édition gros buffers (syntaxe coupée à 20 Mo) |
| `QDir::entryList` à chaque frappe | `CommandCompleter::pathSuggestions` | Freeze sur gros répertoires |
| `waitForFinished(1000)` + `200` | `TerminalProcess::stop` | Bloque la fermeture (acceptable si court, à documenter) |
| Scan PATH | `CommandDiscovery::scanSystemCommandsAsync` | **OK** (`QtConcurrent`) |
| `--help` commandes | `scanCommandArgumentsAsync` | **OK** (worker), mais `waitForStarted`/`waitForFinished` dans le worker |

Réseau IA et `QProcess` terminal : asynchrones. Git/LSP/indexation : N/A.

### 9.3 Lifetime QObject

Globalement sain (parent-child). Points d’attention :

- `EditorManager` crée `QTabWidget(dialogParent)` : le tab widget est enfant de `MainWindow`, pas de `EditorManager`. Cohérent aujourd’hui, fragile si on déplace le manager.
- `FindReplaceDialog` crée des widgets **sans parent** puis `setLayout` : Qt les réparent, OK.
- `new SyntaxHighlighter(editor->document(), lang)` : parent = document, `delete` manuel au resync. OK.
- `QFutureWatcher` parenté à `CommandDiscovery` + `deleteLater` : OK.
- `ChatRepository` : connexion SQLite nommée UUID, `removeDatabase` dans le destructeur : OK si pas d’accès après close.

### 9.4 Courses / concurrence

- `m_commands` / `m_arguments` de `CommandDiscovery` mutés uniquement dans les slots `finished` (thread objet) : pas de mutex, **OK tant que `suggest()` reste sur le thread UI**.
- `FileWatcher::m_ignoreOnce` : thread UI uniquement. Race possible disque : le watcher peut émettre **après** le `ignoreNextChange` si l’événement arrive trop tard → faux prompt reload. Debounce dirs 250 ms, **pas** de debounce fichiers.
- Pas de document partagé multi-vues → pas de race buffer.

### 9.5 Filesystem / sécurité

`Workspace::createEmptyFile` / `createDirectory` :

- Concatènent `directory` + `fileName` **sans** rejeter `..`, séparateurs, ou chemins absolus.
- `createDirectory(..., "nested/dir")` est **testé et autorisé** (`mkpath`).
- `containsPath` compare des préfixes `cleanPath` **sans** `canonicalFilePath` → **symlinks** peuvent pointer hors workspace.
- `FileExplorer::revealPath` refuse un relatif commençant par `..` : bon, mais ne couvre pas les opérations de création.
- Explorer n’a pas encore rename/delete/paste.

Normalisation existante et à réutiliser : `EditorDocument::normalizePath`, `FileWatcher` (canonical).

### 9.6 Encoding / EOL

- Lecture/écriture via `QIODevice::Text` + `QTextStream` : conversion **locale** des fins de ligne, UTF-8 Qt 6 par défaut mais **pas de BOM**, pas de conservation CRLF vs LF.
- Status bar n’affiche ni encoding ni EOL.
- `AtomicFile` : UTF-8 implicite, `setDirectWriteFallback(true)` (moins atomique sur certains FS).

### 9.7 i18n — chaînes sans `tr()`

**`FindReplaceDialog.cpp` :** `"Find / Replace"`, `"Search text..."`, `"Replace with..."`, `"Case sensitive"`, `"Use Regular Expression"`, `"Find Next"`, `"Replace"`, `"Replace All"`, `"Cancel"`, `"Find:"`, `"Replace:"`, `"Find"`, `"No more matches found."`, `"Replace All"`.

**`Terminal.cpp` :** `"Directory not found: "`, `"A command is already running..."`, `"Process crashed"`, `"Process exited with code %1"`.

**`MainWindow.cpp` :** `setText("▼")` / `"▶"` (symboles, OK) ; titre fenêtre initial UI `"Code Editor"`.

**`MainWindow.ui` :** `"Show lignes"` (typo), `"FindReplace"`, `"Explorer"`, `"Unsupported file type."`.

**`ChatWidget` :** placeholder français hardcodé « Posez votre question à Gemini... » (OK avec `tr()`, mais couple l’UI à Gemini).

Pas de Linguist (`.ts` / `.qm`).

### 9.8 Qualité code ponctuelle

- `ChatWidget::appendMessage` : attribut `style` user **non fermé** avant `<span` → bulles user visuellement cassées.
- `ChatWidget::saveChatHistory()` : no-op ; la persistance réelle est `ChatRepository::append` à chaque message.
- `CodeEditor::swapLineUp/Down` : `document()->toPlainText().mid(...)` — O(n) mémoire.
- `highlightCurrentLine` incompatible avec diagnostics/multi-sélection.
- Couleurs gutter `#2d2d30` indépendantes du thème light.
- Police éditeur par défaut `Consolas` (mauvaise sur Linux/macOS sans fallback).
- `GeminiProvider` met la clé dans le header HTTP (correct) ; ne la loggue pas (correct). Erreurs réseau peuvent contenir le body serveur.

---

## 10. Métriques approximatives

| Métrique | Valeur |
|---|---|
| Fichiers applicatifs `src/` (`*.cpp` `*.h` `*.ui`) | 78 |
| Lignes `src/*.cpp` (approx.) | ~5 200 |
| Lignes `src/*.h` (approx.) | ~1 050 |
| Fichiers tests | 12 |
| Lignes tests (approx.) | ~820 |
| Cibles CMake applicatives | 1 exe + 1 lib vendor |
| Cibles test | 12 |
| Langages Tree-sitter réellement highlightés | 2 (C++/HTML) |
| Langages **déclarés** dans `LanguageId` | 16 |
| Workflows CI | 1 (macOS build+DMG) |
| Fichiers `.clang-format` / `.clang-tidy` | 0 |
| Modules `lsp/` `scm/` `tasks/` `debug/` | 0 |

**Baseline compilation (cette machine) :**

| Config | Résultat | Warnings compilateur observés | CTest |
|---|---|---|---|
| Debug (`cmake --preset debug`) | OK → `build/debug/Editerako.exe` | **0** (rebuild complet cible `Editerako`) | **12/12** (~2.6 s) |
| Release (`cmake --preset release`) | OK → `build/release/Editerako.exe` | **0** | **12/12** (~1.2 s) |

Toolchain : CMake 3.30.5, Ninja, g++ 13.1.0, Qt 6.11.2 mingw_64. Module PdfWidgets présent.

---

## 11. Plan de migration

Ordre **obligatoire** (rappel). Chaque phase : compile + CTest + docs + pas de régression des features §3.

| Phase | Objectif | Prérequis | Risque principal |
|---|---|---|---|
| **0** | Audit + baseline (ce document) | — | — |
| **1** | Targets CMake `EditerakoCore` … + exe | 0 | Casser les tests qui recopient les `.cpp` |
| **2** | `MainWindow` composition root | 1 | Casser session / D&D / watcher |
| **3** | Extraire features `CodeEditor` / alléger `EditorManager` | 2 | Multi-curseur, tabs viewers |
| **4** | Document model + encoding + EOL | 3 | Perte de données save/reload |
| **5** | Settings UI + KeybindingManager | 2, 4 | Raccourcis cassés |
| **6** | Command Palette + Quick Open | 1, 5 | Indexation UI thread |
| **7** | Workspace search + explorer ops sécurisées | 1, 4 | Path traversal |
| **8** | Editing commands (indent, comments, …) | 3, 5 | — |
| **9** | Tree-sitter multi-langues **réelles** | 1 | Taille binaire, queries |
| **10** | LSP générique + tests mock | 1, 4 | Framing JSON-RPC |
| **11** | clangd + completion/hover/nav | 10 | clangd absent |
| **12** | Problems panel | 11 | — |
| **13** | Git CLI + Diff | 1 | Parser git, UI thread |
| **14** | Tasks + CMake CLI | 1 | — |
| **15** | `ITerminalBackend` + PTY | terminal actuel intact | ConPTY/PTY portabilité |
| **16** | IA multi-provider | `IAiProvider` déjà amorcé | Clés, streaming |
| **17** | Recovery / Hot Exit | 4 | Secrets dans backups |
| **18** | Editor groups / split | 3, 4 | Session restore |
| **19** | DAP | 10, 12 | — |
| **20** | Plugins locaux | 1, 5 | API instable |
| **21** | CI matrice, sanitizers, packaging | tout le reste | — |

**Prochaine phase recommandée : Phase 1 — CMake modulaire**, sans changer le comportement utilisateur.

Découpage CMake proposé (à valider en Phase 1, ADR `0001`) :

```
tree_sitter
EditerakoCore          # Logging, AppSettings, SessionStore, AtomicFile, DropPaths, CommandRegistry, ThemeManager
EditerakoSyntax        # LanguageRegistry, TreeSitter, HighlightQuery, SyntaxHighlighter
EditerakoEditor        # CodeEditor, EditorDocument, EditorManager, dialogs
EditerakoProject       # Workspace, FileExplorer, FileWatcher
EditerakoTerminal
EditerakoViewers
EditerakoAI
Editerako              # app + main
```

Les tests lieront ces libs au lieu de recompiler les sources.

---

## 12. Décisions à ne pas « corriger » en passant

Comportements documentés dans `docs/conventions.md` / `architecture.md` :

1. PDF : `setDocument` seulement si `Ready`.
2. Terminal : cwd = dossier du fichier actif, sinon workspace ; boutons × capturent `Terminal*`.
3. Premier `Ctrl+J` : le panneau démarre `setVisible(false)` avec `m_userVisible == true` → le premier toggle aligne l’état.
4. Untitled non restaurés par `SessionStore`.
5. Clé Gemini uniquement via environnement / `.env`, jamais commitée.

---

## 13. Definition of Done — Phase 0

1. Repository analysé (CMake, src, tests, CI, scripts, ressources) — **oui**
2. Debug compilé — **oui**
3. Release compilé — **oui**
4. CTest Debug + Release — **12/12**
5. Warnings notés — **aucun warning compilateur sur cette toolchain**
6. Document de baseline — **ce fichier**
7. Aucune refonte de code — **oui**
