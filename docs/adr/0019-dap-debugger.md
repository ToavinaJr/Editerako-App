# ADR 0019 — Debugger DAP

- **Statut :** accepté
- **Date :** 2026-09-04
- **Phase :** 19

## Contexte

Le cahier des charges demande un débogueur DAP (pas un stub), un panneau Debug / Debug Console, et des breakpoints. DAP n’est **pas** du JSON-RPC 2.0 LSP (`method` / `id`) : les messages portent `seq`, `type` (`request` / `response` / `event`) et `command`. Les phases 10 (framing `Content-Length`, `LspServerProcess`) et 12 (Problems) sont des prérequis de transport et d’UI inférieure, pas un client à réutiliser tel quel.

GDB MinGW 13 n’expose souvent **pas** `--interpreter=dap` (DAP ≈ GDB 14). L’absence d’adaptateur doit afficher une erreur dans la Debug Console, pas un simulateur fictif.

## Décision

- Cible **`EditerakoDap`** (`src/debug/`) : `DapTypes`, `DapClient`, `LaunchFile` (`.editerako/launch.json`), `BreakpointStore`. Dépend de `EditerakoCore` + `EditerakoLsp` (framing / spawn). **Pas de Widgets.**
- Réutiliser `LspMessageFramer` + `ProcessJsonRpcTransport` + `LspServerProcess`. **Ne pas** réutiliser `LspClient`.
- **`DebugSession`** (`src/app/`) : composition root côté session (comme `LspSession`). États Idle / Starting / Running / Stopped / Terminated. Handshake : `initialize` → `launch`/`attach` → event `initialized` → `setBreakpoints` → `configurationDone` si supporté.
- Adaptateurs : `gdb --interpreter=dap` (`type` `gdb` / `cppdbg`), `lldb-dap` (`type` `lldb`), ou `adapterCommand` / `adapterArgs` explicites.
- Variables : `${workspaceFolder}`, `${workspaceRoot}`, `${file}`, `${fileDirname}`.
- UI : onglet **Debug** dans `BottomPanel` (call stack, variables, console + REPL `evaluate`). Gutter : clic = breakpoint ; ligne d’arrêt jaunie. `Ctrl+Shift+D` est déjà Duplicate Line : le panneau se bascule avec **`Ctrl+Shift+Y`**.
- Raccourcis : F5 start/continue, Shift+F5 stop, F9 breakpoint, F10 / F11 / Shift+F11 step.

Les tests parlent à un `FakeJsonRpcTransport`. Aucun binaire gdb/lldb n’est requis.

## Conséquences

- gdb trop ancien : process start ou `initialize` échoue ; message clair, session Idle.
- Reverse requests (`runInTerminal`) : réponse `success: false`.
- `launch.json` est local (`.editerako/` gitignoré), comme `tasks.json`.
