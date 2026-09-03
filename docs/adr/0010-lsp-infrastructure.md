# ADR 0010 — Infrastructure LSP

- **Statut :** accepté
- **Date :** 2026-09-03
- **Phase :** 10

## Contexte

Le cahier des charges impose un module `src/lsp/` générique (JSON-RPC 2.0, `Content-Length`, messages partiels) **avant** clangd, la completion UI et le Problems panel. Le transport ne doit pas dépendre des widgets.

## Décision

Cible `EditerakoLsp` (statique), dépendances `EditerakoCore` + `Qt6::Core`. Aucun `#include` Widgets dans `src/lsp/`.

| Type | Rôle |
|---|---|
| `LspMessageFramer` | `Content-Length` + buffer partiel (`\r\n\r\n` ou `\n\n`) |
| `JsonRpcTransport` / `ProcessJsonRpcTransport` | JSON-RPC sur `QIODevice` / `QProcess` |
| `LspClient` | requêtes id, notifications, `initialize`/`initialized`, réponses aux server requests (`result: null`) |
| `LspServerProcess` | spawn async, pas de crash si la commande est vide ou absente |
| `LspServerManager` | specs, une instance partagée par langage, `attachTransport` pour les tests |
| `LspDocumentSync` | `didOpen` / `didChange` (full) / `didSave` / `didClose` |
| Providers | diagnostics (notification), completion, hover, definition/references/rename, document/workspace symbols |
| `LspTypes` | Position/Range/Location/Diagnostic/CompletionItem/Hover/Symbol + URI `file:` |

`MainWindow` crée le manager (composition root) et l’arrête au close. **Aucun serveur n’est démarré** (clangd = phase 11). Pas de popup completion, pas de gutter diagnostics, pas de Problems panel.

Les tests parlent à un `FakeJsonRpcTransport` in-process. Pas de binaire LSP requis.

## Conséquences

- clangd absent : aucun impact en phase 10.
- `waitForStarted` n’est pas utilisé au start (thread UI). Commande vide ou binaire introuvable : `start()` retourne `false` et émet `failed` sans lancer `QProcess`. Un crash ultérieur du process passe encore par `errorOccurred` → `failed`.
- `languageServer` dans `LanguageDefinition` n’est pas encore consommé.
