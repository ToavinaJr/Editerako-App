# ADR 0004 — Document model, encoding et EOL

- **Statut :** accepté
- **Date :** 2026-09-03
- **Phase :** 4

## Contexte

La lecture/écriture passait par `QIODevice::Text` + `QTextStream` : conversion locale des fins de ligne, UTF-8 sans conservation du BOM, pas de modèle encoding/EOL sur `EditorDocument`. Un save/reload pouvait changer CRLF en LF (ou l’inverse) et perdre un BOM.

## Décision

- Détection et (ré)encodage dans `core/TextFileFormat` : UTF-8, UTF-8 BOM, UTF-16 LE/BE, repli ISO-8859-1. LF / CRLF / CR. En mémoire le texte est toujours **LF**.
- I/O binaire : `writeBytesAtomically` ; `readTextFile` / `writeTextFile` posent le `TextFileMeta` sur `EditorDocument`.
- `EditorDocument` porte aussi language (via le chemin), read-only, version interne, caret (position / sélection / scroll).
- Status bar permanente : Ln/Col, encoding, EOL, language (`EditorStatusWidget`). Pas de Git/LSP (modules absents).
- Fichier *untitled* : UTF-8, EOL plateforme (CRLF sur Windows — même effet qu’avant `QIODevice::Text`).
- Latin1 incapable de représenter un caractère Unicode → upgrade UTF-8 à l’écriture (évite une perte).
- UTF-32 avec BOM : décodé puis **sauvegardé en UTF-8** (format rare, pas d’enum dédié).

**Non fait :** séparation Document / EditorView multi-vues (Phase 18), picker d’encodage, compare-with-disk UI (la fonction `diskMatches` existe), recovery (Phase 17).

## Conséquences

- Ne plus ouvrir les fichiers texte avec `QIODevice::Text`.
- Un fichier LF Linux ouvert sous Windows reste LF à la sauvegarde.
- `writeTextAtomically` écrit de l’UTF-8 brut (LF inchangé) ; l’éditeur passe par `writeTextFile`.
