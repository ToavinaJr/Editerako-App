# ADR 0016 — Chat compte (sign-in) et providers API

- **Statut :** accepté
- **Date :** 2026-09-04
- **Phase :** 16

## Contexte

Le chat était collé à **Gemini API** (`GEMINI_API_KEY` obligatoire, UI « GEMINI AI »). Le cahier des charges demande un multi-provider. L’usage voulu n’est pas d’imposer Gemini : l’utilisateur se connecte avec **son vrai compte** (ChatGPT, Claude, Gemini Google, Copilot) et retrouve **ses** conversations.

Qt WebEngine n’est pas dans le kit MinGW. Edge WebView2 est présent sur Windows.

## Décision

- Défaut : provider `chatgpt` (**compte**, pas l’API Gemini).
- Panneau chat : combo **Sign in — …** (WebView2 + profil cookies sous `%AppData%/Editerako/webview-profile`) et **API — …** (Gemini, OpenAI, Anthropic, Ollama).
- Bouton **Sign in** : session dans le navigateur embarqué, ou navigateur système si WebView2 indisponible. Pas de clé API pour ce mode.
- Gemini API, OpenAI, Anthropic, Ollama restent disponibles. Clés uniquement via `.env` / l’environnement (`GEMINI_API_KEY`, `OPENAI_API_KEY`, `ANTHROPIC_API_KEY`). Ollama : pas de clé.
- `AiProvider::create` retourne `nullptr` pour un provider compte. `cancel()` / `isBusy()` sur les backends HTTP.
- Parseurs JSON extraits (`AiResponseParse`) pour les tests sans réseau.

## Conséquences

- Un utilisateur existant qui a déjà `ai/provider=gemini` dans QSettings **garde** l’API Gemini.
- Le SDK WebView2 (NuGet) est téléchargé dans `build/_deps/webview2` ; `WebView2Loader.dll` est copié à côté de `Editerako.exe`.
- Linux/macOS : pas d’embed WebView2 ; fallback « ouvrir dans le navigateur ».
- Streaming SSE : parseur testé (`parseOpenAiSseDelta`), pas encore branché sur le fil HTTP (hors scope de cette livraison).
