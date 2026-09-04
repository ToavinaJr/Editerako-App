# ADR 0017 — Recovery / Hot Exit

- **Statut :** accepté
- **Date :** 2026-09-04
- **Phase :** 17

## Contexte

La session restaurait workspace, onglets **existants** et géométrie. Les buffers dirty et les *untitled* disparaissaient après un crash, une coupure, ou une fermeture. Un backup dans le workspace (`{projet}/.editerako`) risquait d’être commité avec des secrets.

## Décision

- **`BackupService`** écrit un snapshot atomique (`index.json` + `*.txt`) sous `%LOCALAPPDATA%/Editerako/Editerako/backups` (`QStandardPaths::AppLocalDataLocation`), jamais dans le dépôt.
- **`RecoveryService`** charge / ignore / efface ce snapshot. Restauration silencieuse au prochain lancement (barre de statut).
- Période : debounce 1 s après modification (et à la sauvegarde / à la fermeture). Les fichiers originaux ne sont **pas** écrasés.
- **Hot Exit** (`files/hotExit`, défaut **true**) : quitter sans dialogue Save All ; les unsaved reviennent au relance. Désactivé : dialogue actuel, puis **discard** du snapshot (un Discard volontaire ne doit pas ressusciter les buffers).
- Secrets **non** sauvegardés : `.env` / `.env.*` (sauf `.env.example`), `*.env`, `credentials.json`, clés `id_rsa` / `*.pem` / `*.key`, etc. S’il reste des secrets dirty à la fermeture, un prompt dédié s’affiche.
- Plafond 8 Mio UTF-8 par buffer. Identifiants d’entrée validés (pas de path traversal).

## Conséquences

- Les untitled non sauvegardés survivent à un crash / Hot Exit.
- Un `.env` modifié n’est jamais copié dans AppData ; l’utilisateur doit le sauver ou le jeter explicitement.
- Le snapshot n’est pas versionné avec le projet.
