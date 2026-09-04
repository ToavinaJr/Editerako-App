# ADR 0015 — Backend terminal et PTY

- **Statut :** accepté
- **Date :** 2026-09-04
- **Phase :** 15

## Contexte

Le terminal historique lance chaque commande dans un `QProcess` one-shot (shell `cmd /c`, PowerShell `-Command`, Unix `-c`), avec builtins `cd` / `pwd` / `clear`. Le cahier des charges demande un `ITerminalBackend`, un fallback process, et un PTY (ConPTY Windows / PTY Unix) pour que `isatty` soit vrai et que les séquences ANSI (couleurs) arrivent.

Transformer le widget en émulateur VT interactif (session shell persistante, double prompt) casserait l’UX actuelle.

## Décision

- `ITerminalBackend` : `start` / `write` / `resize` / `stop`, signaux `dataReceived`, `finished`, `failed`.
- `ProcessTerminalBackend` : comportement actuel (`QProcess` + `COLUMNS` / `LINES` / `TERM`).
- `PtyTerminalBackend` : ConPTY via `GetProcAddress` (Windows 10 1809+) ou `posix_openpt` (Unix). Indisponible → fallback process.
- `TerminalProcess` choisit PTY seulement si `terminal/usePty` **et** `PtyTerminalBackend::isAvailable()`. Défaut : **false** (UX inchangée).
- `AnsiSgrDecoder` colorie la sortie (SGR 0/1/30–37/90–97/39) et ignore les autres CSI. `clear` réinitialise l’état.
- Profils de shell : `detectShellProfiles()` / `defaultShellPath()` / `shellCommandArguments()`. Combo éditable dans Préférences.

## Conséquences

- Le prompt ligne + builtins restent dans `Terminal`. Pas de session PTY persistante dans cette phase.
- Resize : estimation cols/rows via `QFontMetrics` du `terminalOutput`, transmise au backend.
- Overlay workspace : `terminal.usePty` (bool). Les clés API ne sont pas concernées.
- Un test ConPTY live depuis un exe console (ctest) n’est pas fiable : l’enfant s’attache à la console parent. `test_PtyTerminalBackend` vérifie `isAvailable` sous Windows et l’echo réel sous Unix. Le chemin ConPTY est celui de l’app WIN32.
