# ADR 0009 — Tree-sitter multi-langues

- **Statut :** accepté
- **Date :** 2026-09-03
- **Phase :** 9

## Contexte

`LanguageRegistry` déclarait C, CMake, CSS, JS, TS, TSX, JSON, Markdown, Python, Shell, SQL et YAML, mais seules les grammaires **C++** et **HTML** étaient compilées. C réutilisait `tree_sitter_cpp()`. Un langage ne doit pas être « supporté syntaxiquement » si seule son extension est reconnue.

## Décision

1. **Vendor** les grammaires officielles (sources `parser.c` / `scanner.c` + LICENSE) sous `tree-sitter/`. Pins : `scripts/vendor-tree-sitter-grammars.py` et `tree-sitter/grammars.json`. SQL : artefacts `gh-pages` de DerekStride (le `parser.c` n’est pas sur `main`).
2. **Une OBJECT library CMake par grammaire** (`cmake/TreeSitter.cmake`) pour isoler les includes des scanners. Runtime ABI 15 / min 13 ; grammaires LANGUAGE_VERSION 14 ou 15.
3. **`LanguageDefinition`** : table unique (`id`, `displayName`, `extensions`, `filenames`, `treeSitterLanguage`, `highlightQuery`, `commentTokens`, `brackets`, `indentTriggers`, `languageServer`). Plus de `switch` métier dans le registre.
4. **C** a sa propre grammaire (`tree_sitter_c`) et `resources/syntax/c/highlights.scm`. `.h` reste C++.
5. Queries embarquées, captures alignées sur `SyntaxHighlighter` (`comment`, `string`, `keyword`, …). Predicates `#lua-match?` exclus (non implémentés).
6. Markdown : grammaire **block** seulement (pas d’injections inline). Shell → `tree-sitter-bash`.

`languageServer` est de la metadata (ex. `clangd` pour C/C++) ; le client LSP est la phase 10.

## Conséquences

- Le binaire et le temps de compile augmentent (SQL ~1,4 M lignes de `parser.c`).
- Highlight Markdown des spans inline (gras, liens) : phase ultérieure (grammaire inline + injections).
