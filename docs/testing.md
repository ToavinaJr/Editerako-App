# Tests

Suite **Qt Test** + **ctest**. On teste la logique isolée, pas `MainWindow` ni les widgets lourds (terminal, PDF).

```powershell
.\scripts\test.ps1
```

```bash
./scripts/test.sh
```

Équivalent : `ctest --preset debug --output-on-failure`.

Sanitizers : `./scripts/test.sh asan` ou `tsan` (Linux/macOS). Presets et CI : [ci.md](ci.md).

## Cibles actuelles

Les tests **lient** la bibliothèque du module (`EditerakoCore`, …). Ils ne recompilent pas les `.cpp` métier.

| Cible | Module lié | Fichiers sous test |
|---|---|---|
| `test_LanguageRegistry` | `EditerakoSyntax` | extensions, `LanguageDefinition`, pointeurs Tree-sitter pour chaque langage |
| `test_Workspace` | `EditerakoProject` | racine, `containsPath`, exclusions, création fichier/dossier |
| `test_FileExplorerDecorations` | `EditerakoProject` | icônes d’extension, couleurs de badges SCM, libellé d’item |
| `test_WorkspaceController` | `EditerakoProject` | `setRootPath`, create file/folder (`QT_QPA_PLATFORM=offscreen`) |
| `test_SessionController` | `EditerakoCore` | restore guard, fichiers existants, workspace restorable |
| `test_DiskChangePolicy` | `EditerakoCore` | deleted/dirty → action |
| `test_DropPaths` | `EditerakoCore` | extraction de chemins depuis `QMimeData` |
| `test_SessionStore` | `EditerakoCore` | round-trip via `QSettings` Ini temporaire |
| `test_RecentWorkspaces` | `EditerakoCore` | mémorisation, plafond 10, prune des dossiers disparus |
| `test_ContextBuilder` | `EditerakoAI` | prompt, troncature, fenêtre d’historique |
| `test_AiCatalog` | `EditerakoAI` | défaut compte ChatGPT, kinds account/API |
| `test_AiResponseParse` | `EditerakoAI` | JSON Gemini / OpenAI / Anthropic, SSE |
| `test_FileKind` | `EditerakoViewers` | texte / PDF / image / SVG / CSV / Markdown reste texte |
| `test_CsvParser` | `EditerakoViewers` | virgules, quotes, CRLF, newline quoté |
| `test_BuiltinViewers` | `EditerakoViewers` | load Markdown / SVG / CSV (`offscreen`) |
| `test_CommandRegistry` | `EditerakoCore` | enregistrement, doublons, `setEnabled` (`QT_QPA_PLATFORM=offscreen`) |
| `test_AtomicFile` | `EditerakoCore` | écriture atomique, Unicode, octets bruts |
| `test_TextFileFormat` | `EditerakoCore` | UTF-8/BOM, UTF-16, Latin1, LF/CRLF, upgrade Unicode |
| `test_AppSettings` | `EditerakoCore` | round-trip user, overlay workspace JSON, `ui/language` |
| `test_TranslationLoader` | `EditerakoCore` | locale, catalogues, `.ts` français (`EDITERAKO_TRANSLATIONS_DIR`) |
| `test_KeybindingModel` | `EditerakoCore` | défauts, override, conflits, apply sur `CommandRegistry` (`QT_QPA_PLATFORM=offscreen`) |
| `test_FuzzyMatcher` | `EditerakoCore` | sous-séquence, score, `fichier:ligne` |
| `test_WorkspaceFileIndex` | `EditerakoProject` | exclusions, rebuild async (`indexUpdated`) |
| `test_WorkspacePath` | `EditerakoProject` | `..`, chemins absolus, `nested/dir` |
| `test_GitIgnore` | `EditerakoProject` | globs, `*.log`, négation, `/` ancré, `.git` |
| `test_WorkspaceSearch` | `EditerakoProject` | texte / regex / whole word / replace |
| `test_WorkspaceOps` | `EditerakoProject` | rename, duplicate, copy, delete, refus racine |
| `test_CommandHistory` | `EditerakoTerminal` | historique, navigation |
| `test_CommandCompleter` | `EditerakoTerminal` | suggestions commande / args / chemin |
| `test_AnsiSgr` | `EditerakoTerminal` | SGR couleurs, strip, état entre chunks |
| `test_ShellProfiles` | `EditerakoTerminal` | shell par défaut, arguments cmd / PowerShell / Unix |
| `test_ProcessTerminalBackend` | `EditerakoTerminal` | echo via `QProcess` |
| `test_PtyTerminalBackend` | `EditerakoTerminal` | `isAvailable`, echo Unix PTY (ConPTY live skip sous console Windows) |
| `test_TreeSitterDocument` | `EditerakoSyntax` | parse incrémental (`QT_QPA_PLATFORM=offscreen`) |
| `test_FoldRanges` | `EditerakoSyntax` | plages fold C++ / JSON depuis l’arbre (`offscreen`) |
| `test_FoldingController` | `EditerakoEditor` | fold all / unfold / toggle (`offscreen`) |
| `test_HighlightQuery` | `EditerakoSyntax` | captures, predicates, queries SCM de tous les langages (`EDITERAKO_QUERY_DIR`) |
| `test_EditorIo` | `EditerakoEditor` | lecture texte, Unicode, conservation CRLF, `diskMatches` |
| `test_HighlighterSync` | `EditerakoEditor` | `shouldHighlight` : Python / C++ / HTML, seuil gros fichier |
| `test_LineMovementController` | `EditerakoEditor` | swap up/down sur `QPlainTextEdit` (`QT_QPA_PLATFORM=offscreen`) |
| `test_IndentOps` | `EditerakoEditor` | indent/outdent, smart indent, convert, trim, sort |
| `test_CommentOps` | `EditerakoEditor` | toggle ligne / bloc |
| `test_BracketMatcher` | `EditerakoEditor` | matching `()[]{}`, auto-close |
| `test_LineEditCommands` | `EditerakoEditor` | duplicate/delete/join/sort, occurrence (`offscreen`) |
| `test_MultiCursorController` | `EditerakoEditor` | toggle, insert multi, doublon du curseur primaire (`QT_QPA_PLATFORM=offscreen`) |
| `test_EditorDocument` | `EditerakoEditor` | format par défaut, language, version, caret (`QT_QPA_PLATFORM=offscreen`) |
| `test_EditorArea` | `EditerakoEditor` | split horizontal / vertical, unwrap (`offscreen`) |
| `test_TabOps` | `EditerakoEditor` | Close to Right / Close Saved / Close Others (skip pinned) |
| `test_EditorGroup` | `EditerakoEditor` | pin à gauche, preview exclusif, clic du milieu (`offscreen`) |
| `test_LspMessageFramer` | `EditerakoLsp` | `Content-Length`, messages partiels |
| `test_LspTypes` | `EditerakoLsp` | Position, diagnostics, hover, completion, URI |
| `test_LspPickerItems` | `EditerakoLsp` | labels / hints / fallback path des pickers définition & symboles |
| `test_LspCompileCommands` | `EditerakoLsp` | localisation de `compile_commands.json` (racine, `build/<preset>`) |
| `test_LspHoverHtml` | `EditerakoLsp` | échappement des templates, couleurs C++, lien Go to Definition |
| `test_LspClient` | `EditerakoLsp` | initialize mock, didOpen, diagnostics, providers |
| `test_LspServerProcess` | `EditerakoLsp` | commande vide, binaire absent, spec inconnue |
| `test_CompletionModel` | `EditerakoEditor` | filtre, sortText, insert / textEdit (`offscreen`) |
| `test_DiagnosticMarkup` | `EditerakoEditor` | underline range, clamp position, gutter (`offscreen`) |
| `test_ProblemModel` | `EditerakoEditor` | store par fichier, filtres Errors/Warnings, counts |
| `test_GitParsers` | `EditerakoScm` | porcelain `-z`, rename, MM, chemins absolus |
| `test_TextDiff` | `EditerakoScm` | LCS, unified, EOL |
| `test_GitCliProvider` | `EditerakoScm` | non-repo, stage (skip si `git` absent) |
| `test_TaskFile` | `EditerakoTasks` | parse `tasks.json`, variables `${workspaceFolder}` |
| `test_CMakePresets` | `EditerakoTasks` | presets visibles, `binaryDir`, specs CLI |
| `test_ProblemMatcher` | `EditerakoTasks` | gcc / msvc |
| `test_TaskManager` | `EditerakoTasks` | tasks custom + CMake, runner echo |
| `test_DapClient` | `EditerakoDap` | framing DAP, initialize mock |
| `test_LaunchFile` | `EditerakoDap` | parse `launch.json` |
| `test_BreakpointStore` | `EditerakoDap` | breakpoints par fichier |
| `test_DapTypes` | `EditerakoDap` | séquence, events, variables |
| `test_PluginManifest` | `EditerakoPlugins` | parse `plugin.json`, `apiVersion`, refus `library` hors dossier |
| `test_PluginManager` | `EditerakoPlugins` | chargement JSON, désactivation, `IPlugin` in-process (`offscreen`) |

Les binaires sont dans `build/<preset>/tests/`.

## Ajouter un test

1. Créer `tests/FooTest.cpp` :

```cpp
#include "module/Foo.h"
#include <QtTest>

class FooTest : public QObject
{
    Q_OBJECT
private slots:
    void example();
};

void FooTest::example()
{
    QCOMPARE(1 + 1, 2);
}

QTEST_GUILESS_MAIN(FooTest)   // QTEST_MAIN si QWidget / QApplication
#include "FooTest.moc"
```

2. Dans `tests/CMakeLists.txt` :

```cmake
editerako_add_test(test_Foo
    SOURCES FooTest.cpp
    LIBS EditerakoCore
)
```

Lier la cible du module (`EditerakoCore`, `EditerakoSyntax`, …), pas les `.cpp` individuels, et pas `Editerako.exe`.

3. Si le test crée un `QWidget`, utiliser `QTEST_MAIN`, et :

```cmake
editerako_test_offscreen(test_Foo)
```

4. `.\scripts\test.ps1` — tout doit rester vert.

Isoler le disque et `QSettings` avec `QTemporaryDir`. Ne pas écrire dans le profil `Editerako` de l’utilisateur.
