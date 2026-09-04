# ADR 0001 — Cibles CMake modulaires

- **Statut :** accepté
- **Date :** 2026-09-03
- **Phase :** 1

## Contexte

Les dossiers sous `src/` étaient des modules logiques regroupés dans un seul `qt_add_executable(Editerako)`. Les tests Qt Test **recompilaient** les `.cpp` métier au lieu de lier une bibliothèque. Les dépendances Qt (Network, Sql, Concurrent, PdfWidgets, Tree-sitter) n’étaient visibles que sur l’exécutable final.

## Décision

Introduire des bibliothèques **statiques** internes, une par module, créées via `editerako_add_module()` (`cmake/Libraries.cmake`) :

| Cible | Dossier | Dépendances PUBLIC |
|---|---|---|
| `EditerakoCore` | `src/core/` | `Qt6::Widgets` |
| `EditerakoSyntax` | `src/syntax/` | `EditerakoCore`, `tree_sitter`, `Qt6::Widgets` |
| `EditerakoEditor` | `src/editor/` | `EditerakoCore`, `EditerakoSyntax`, `Qt6::Widgets` |
| `EditerakoProject` | `src/project/` | `EditerakoCore`, `Qt6::Widgets` |
| `EditerakoTerminal` | `src/terminal/` | `EditerakoCore`, `Qt6::Widgets`, `Qt6::Concurrent` |
| `EditerakoViewers` | `src/viewers/` | `EditerakoCore`, `EditerakoEditor`, `Qt6::Widgets`, `Qt6::PdfWidgets` |
| `EditerakoAI` | `src/ai/` | `EditerakoCore`, `Qt6::Widgets`, `Qt6::Network`, `Qt6::Sql` |
| `EditerakoLsp` | `src/lsp/` | `EditerakoCore`, `Qt6::Core` |
| `EditerakoScm` | `src/scm/` | `EditerakoCore`, `Qt6::Core`, `Qt6::Concurrent` |
| `Editerako` | `src/app/`, `src/ui/`, `main.cpp` | toutes les libs ci-dessus (PRIVATE) |

`tree_sitter` reste une lib vendor isolée (`cmake/TreeSitter.cmake`).

Les tests lient la cible de module (`LIBS EditerakoCore`, …) et ne listent plus les sources métier.

Includes : `target_include_directories(... PUBLIC src/)` pour conserver `#include "module/Fichier.h"`.

Les libs sont **statiques** : un seul binaire à déployer, pas de DLL/so supplémentaire, packaging existant inchangé.

## Conséquences

- Ajouter un `.cpp` dans le `CMakeLists.txt` **du module**, pas dans une liste unique `src/CMakeLists.txt`.
- `EditerakoViewers` dépend de `EditerakoEditor` (tabs PDF/images via `EditorManager`) — couplage existant, à traiter en Phase 3 plutôt qu’ici.
- `EditerakoCore` lie `Qt6::Widgets` parce que `CommandRegistry` / `ThemeManager` en ont besoin. Core n’inclut pas `MainWindow`.
- Les tests d’un module léger (ex. `ContextBuilder`) lient toute `EditerakoAI`. Acceptable en Phase 1 ; un découpage plus fin n’est pas justifié tant que le graphe reste acyclique.

## Alternatives rejetées

- **OBJECT libraries** : moins claires pour `target_link_libraries` transitif et pour Qt AUTOMOC.
- **SHARED / plugins Qt** : change le packaging Windows/Linux ; réservé à une phase plugins.
- **Une mega-lib `EditerakoLib`** : ne rend pas les dépendances inter-modules explicites.
