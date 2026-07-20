# AI Functions

romHEX 14 includes an AI assistant that can analyse maps, propose and apply edits, and label characteristics, plus a set of one-click tuning functions. These features connect to an AI provider and, for the tuning functions and translation, require a Pro account.

!!! info "What needs Pro, and what needs a key"
    The **AI Assistant chat panel** runs on an API key that *you* supply — it works in any build once a key is configured. The **AI Functions** dialog (one-click tuning presets) and **AI Translate** (map-name translation) are **Pro** features that require a signed-in account with the matching module.

## Setting up your AI provider

Open the AI provider settings from the AI Assistant panel's overflow menu (**API key & provider…**) or from **Settings ▸ AI**. Configure:

- **Provider** — Claude (Anthropic), OpenAI, Qwen, DeepSeek, Gemini, Groq, Ollama (local), LM Studio (local), or a custom OpenAI-compatible endpoint. A coloured dot shows each provider's support tier: **green** (best — native API, full tool-calling and streaming), **amber** (good — OpenAI-compatible), **red** (limited — compatibility varies).
- **API Key** — your key for that provider, stored locally with obfuscation.
- **Model** — defaults to the provider's recommended model; override if you want a specific one.
- **Base URL** — editable for OpenAI-compatible and local providers (fixed for Claude).

If no key is set, the assistant shows a banner with a **Set API key** button.

## The AI assistant dock

Open the panel with **View ▸ AI Assistant** (`Ctrl+\`) or the **AI** toolbar button. It is a chat interface with access to the active project and all open projects, and it tracks the map you have selected. The welcome screen offers suggestion chips grouped as **Tune**, **Analyze**, **Review** and **Log**.

Use it to ask questions ("What does this map do?", "Show all modified maps"), request analysis ("Detect anomalies in modified maps", "Describe the shape of the selected map"), or request edits in plain language. A project selector in the header lets you point the assistant at a specific open project. The header overflow menu (⋯) also offers the **Tuning logbook & dyno history**, a **Verbose mode** toggle and **Clear conversation**.

## AI Functions dialog

**Project ▸ AI Functions…** opens **AI Tuning Functions** (Pro; requires an open project with map definitions and the `ai_functions` module). It is a grid of one-click presets, each of which searches for the relevant maps, shows you a review, and applies the change:

- **Decat / Catalyst Off**
- **DPF Delete**
- **EGR Off**
- **AdBlue / SCR Off**
- **Swirl Flap Delete**
- **Speed Limiter Off**
- **Start-Stop Disable**

An **Apply to** selector chooses which open project (its current ROM) the function targets. Each function walks through a risk notice, a review tree where you can include/exclude individual maps and adjust the new value, and a final confirmation (large changes require typing **APPLY**), with the option to take a version snapshot first.

## Map analysis

Through the chat, the AI can call read-only analysis tools — describing a map's shape, identifying its likely purpose, finding related maps, comparing against the original or another ROM, summarising all differences, and flagging anomalies in your changes. These never modify the ROM; they help you understand it before you edit.

## Auto-labelling characteristics

The **✦ AI Translate** button in the Map Selection panel's title bar translates map names using AI (Pro; requires the `translation` module and sign-in). Use it to turn cryptic identifiers into readable names for the whole ROM, or right-click a group and choose **✦ AI Translate Group…** to translate just that folder. When you are not signed in the button is hidden; when the module is inactive it is disabled with an explanatory tooltip.

## Multi-project AI runs

The chat assistant is aware of every open project and can target linked ROMs and other open projects through its tools (listing linked ROMs, selecting a target ROM, comparing with a linked ROM, applying a delta). The AI Functions dialog applies to one selected project's current ROM at a time, iterating across many maps within it. Together these let you carry a change across a set of related files while keeping each project's history intact.

## Tool execution and safety

The assistant works through a set of tools (around 52) covering reading and searching maps, comparing and analysing, editing (set/scale/offset/clamp/smooth/zero/fill), axis edits, linked-ROM operations, and version control. Several guardrails protect your data:

- **Permission mode** (in the panel header) cycles through **Ask** (confirm before each change — the default), **Auto** (auto-accept edits) and **Plan** (describe only, apply nothing).
- In **Ask** mode, write operations surface a confirmation card ("AI proposes a change") with **Reject** and **Accept & Apply**, and destructive operations add an extra warning to consider a snapshot first.
- In **Plan** mode, the AI is prevented from writing and simply describes what it would do.
- **Automatic version snapshot** — before each edit the assistant snapshots the ROM (labelled "AI: …"), so every AI change is reversible via the Versions list or an undo tool.

!!! note
    Not every write tool routes through the inline confirmation card in the current build. Review AI edits before saving and exporting, and rely on version snapshots and **Differences vs Original** to see exactly what changed. Always verify the checksum before flashing an AI-assisted tune.
