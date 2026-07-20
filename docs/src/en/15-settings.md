# Settings & Localization

Configure romHEX 14: language, colour theme, the AI provider and a few behaviour options. Most preferences live in the **Configuration** dialog; language and auto-save are on the **Miscellaneous** menu.

## Opening the Configuration dialog

Choose **Miscellaneous ▸ Settings…** (also reachable from the **Command Palette**, `Ctrl+K`). The **Configuration** dialog has a left navigation list with three pages — **Colors**, **Display** and **AI** — and **Reset Defaults**, **Cancel** and **Apply** buttons. Changes take effect when you click **Apply** (except theme presets and colour swatches, which apply immediately).

## Changing the language

romHEX 14 ships in four languages. Switch from the **Miscellaneous ▸ Language** submenu:

- **English**
- **简体中文 (Chinese Simplified)**
- **Español (Spanish)**
- **ภาษาไทย (Thai)**

The change is **immediate — no restart is required**. All menus, panels and dialogs re-translate on the spot, and the interface font switches automatically to a suitable face for the chosen script.

!!! note
    A separate "AI Translation" dialog also has a language dropdown, but that controls the AI translation of *map names*, not the interface language. Use the Language submenu for the UI.

## Theme and appearance

The **Colors** page controls the whole look:

- **Theme Preset** — pick from built-in themes such as *Midnight (Default)*, *Ocean Deep*, *Dark Forest*, *Nord*, *Dracula*, *Gruvbox Dark*, *Solarized Dark*, *Monokai*, *Amber CRT* and *Paper Light*. Selecting one applies immediately.
- **Individual colours** — grouped swatches let you fine-tune every element: the five **Map Highlight Bands** (used for map regions in the hex, 2D and overlay views), the eight **2D curve colours**, and the colours of the hex editor, map overlay, waveform view, general UI and bars. Click any swatch to open a colour picker; the preset switches to **Custom** once you change one.

The **Display** page currently offers one option: **Show long map names (description)** — when enabled, the map list shows the full description (e.g. "Kennfeld Momentenindizierter Motor") instead of the short identifier (e.g. "KFMIOP").

## File paths

romHEX 14 does not use a configurable working-folder setting. Projects are self-contained `.rx14proj` files you save wherever you like, and the Project Manager keeps a registry of the ones you have opened. Recently used files and dialog locations are remembered automatically. Application preferences are stored with the operating system's native settings mechanism (organisation **CT14**, application **romHEX14**), not in a config file you edit by hand.

## AI provider configuration

The **AI** page (mirrored in the AI Assistant's own settings) configures the provider used by the AI features:

- **Provider** — Claude (Anthropic), OpenAI, Qwen, DeepSeek, Gemini, Groq, Ollama (local), LM Studio (local) or a custom OpenAI-compatible endpoint. A coloured dot indicates each provider's support tier (green = best, amber = good, red = limited).
- **API Key** — your own key for the chosen provider (stored locally, obfuscated).
- **Model** — defaults to the provider's recommended model; override if needed.
- **Base URL** — editable for OpenAI-compatible and local providers (fixed for Claude).

The AI *tuning functions* and *map translation* additionally require a Pro account, but the chat assistant runs on the key you provide here. See [AI Functions](13-ai-functions.md).

## Advanced settings

A few behaviour options live outside the Configuration dialog:

- **Auto Save** (**Miscellaneous ▸ Auto Save**) — choose **Off** (manual save only, `Ctrl+S`), **After Delay** (5 seconds after your last edit — the default), **On Focus Change** (when you switch projects) or **On Window Deactivate** (when romHEX 14 loses focus).
- **Auto-scan ROM on import** (**Miscellaneous ▸ Auto-scan ROM on import**) — controls whether a new ROM is scanned for maps automatically.
- **AI permission mode** — Ask / Auto / Plan, set from the AI Assistant panel rather than this dialog.
