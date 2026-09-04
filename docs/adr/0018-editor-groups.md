# ADR 0018 — Editor groups / split

- **Statut :** accepté
- **Date :** 2026-09-04
- **Phase :** 18

## Contexte

Un seul `QTabWidget` dans `EditorManager` : pas de split, pas de vue multiple du même buffer. Les viewers PDF/image vivaient déjà dans ces onglets. Le cahier des charges demande `EditorArea` / `EditorGroup`, Split Right / Down, Move Editor, Close Group, tout en restant identique **sans** split.

## Décision

- **`EditorGroup`** : un `QTabWidget` (reorder, close, clic du milieu, menu contextuel).
- **`EditorArea`** : arbre de `QSplitter` (horizontal / vertical, y compris imbriqué). Un groupe unique = pas de splitter.
- Split d’un **éditeur texte** : deuxième `CodeEditor` qui **partage** le `QTextDocument` / `EditorDocument`. LSP : `didOpen` une seule fois (`m_openUris`) ; `attachEditor` par vue.
- Split d’un **viewer** : `ViewerManager::openNew` (deuxième instance), car `EditerakoEditor` ne dépend pas de `EditerakoViewers`.
- Move Editor : déplace l’onglet vers le groupe suivant ; s’il n’y en a qu’un, crée un split à droite.
- Close Group : ferme les onglets du groupe actif (prompts si dernière vue dirty). Le dernier groupe n’est pas retiré.
- Session / Hot Exit : **pas** de persistance du layout split. Relance = un groupe, mêmes fichiers.

Raccourcis : `Ctrl+\` Split Right, `Ctrl+Shift+\` Split Down. Command Palette / menu View.

## Conséquences

- `EditorManager::containerWidget()` va dans le `centralStack` ; `tabWidget()` reste le groupe **actif**.
- Fermer une vue clone ne demande pas de sauver et n’envoie pas `didClose`.
- Diagnostic LSP recopié sur toutes les vues du chemin.
