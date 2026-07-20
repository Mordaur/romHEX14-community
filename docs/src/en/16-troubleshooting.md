# Troubleshooting

Common problems and how to fix them.

## Application won't start

- **Missing Qt runtime (Linux):** ensure the Qt 6 libraries the build depends on are installed, or use the bundled package.
- **macOS "unidentified developer" warning:** open the app from Applications (right-click ▸ Open the first time) after dragging it from the `.dmg`.
- **Reset a bad state:** romHEX 14 stores its preferences with the OS settings mechanism (organisation **CT14**, application **romHEX14**). If a corrupt setting prevents startup, clearing that store returns the app to defaults (you will lose theme/language/AI preferences, not your projects).
- Your projects are safe regardless — they are standalone `.rx14proj` files, not stored inside the application.

## A2L import fails

- **"Cannot open A2L file":** the path is unreadable or locked — check the file.
- **Most maps out of bounds / low compatibility score:** the A2L does not match this ROM. It was written for a different firmware version. Import it onto the matching original ROM, then use **Link ROM to Project…** to carry the maps across.
- **A map you expected is missing:** ASCII/string characteristics are not imported, and malformed blocks are skipped. Check the A2L defines the object as a VALUE/CURVE/MAP/VAL_BLK characteristic.
- **Values look shifted:** confirm you loaded the ROM that belongs to this A2L; the base address is auto-detected from the A2L.

See [A2L Import](06-a2l-import.md).

## OLS import shows wrong values

- **A whole map reads as noise:** OLS files carry byte order **per map** — an individual record can be re-specified in **Map Properties ▸ Data organization** (LoHi = little-endian, HiLo = big-endian).
- **You only see one version:** a multi-version OLS import puts Version 0 as the main ROM and the rest under the **Versions** node; open the others with **Open Version in New Window**.
- **Nothing imported ("No versions found"):** the file may be an unsupported layout or not a project container. If it is a `.kp`, use **Import KP…** instead ([KP Import](08-kp-import.md)).

See [OLS Import](07-ols-import.md).

## Checksum mismatch on save

- A mismatch is expected after editing — run **Project ▸ Correct Checksum…** (Pro) before exporting, then **Export ROM…**.
- **"Unsupported for this ECU":** on macOS/Linux only the built-in families (EDC15, ME7.x, EDC16, MED17/EDC17, ME9, SIMOS) are available; other ECUs need the Windows DLL modules.
- **Windows: DLL fails to load / "runtime dependency is missing":** install the **Microsoft Visual C++ 2005 SP1 Redistributable (x86)** (the 32-bit version, even on 64-bit Windows).
- **"checksumhelper.exe not found":** the 32-bit bridge is missing — place `checksumhelper.exe` and the `ChecksumDLL` folder next to the executable.

See [Checksum Manager](11-checksum-manager.md).

## AI assistant errors

- **"No API key configured":** open the provider settings and enter your key ([AI Functions](13-ai-functions.md)).
- **"AI Functions requires a Pro account":** the one-click tuning functions and map translation need a signed-in Pro account with the matching module; the chat assistant works with your own key.
- **Requests fail or time out:** check your internet connection and key; for local providers (Ollama, LM Studio) confirm the server is running at the configured Base URL. Red-tier providers have limited tool-calling support — prefer a green-tier provider for reliable edits.
- **An AI edit did something unexpected:** every AI change takes a version snapshot first — revert from the **Versions** list, or use **Differences vs Original** to see exactly what changed.

## Crashes and crash reports

- romHEX 14 saves projects atomically and, by default, auto-saves 5 seconds after your last edit, so little work is lost in a crash.
- On reopening, your last save (or auto-save) is intact in the `.rx14proj`; migrated legacy projects also keep a `.legacy.bak`.
- If a specific file reproducibly crashes the app, note what you were doing and which ROM/A2L/OLS file was involved, and report it (see below).

## Where to get help

- Re-read the relevant chapter — most dialogs are documented here with their exact options.
- Consult the appendices for the menu reference, [file formats](appendix-b-formats.md) and glossary.
- For Pro accounts, licensing and purchases, visit **romhex14.com**.
