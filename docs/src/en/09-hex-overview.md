# Hex Overview

The hex view lets you inspect raw bytes, jump to addresses, visualise map regions and edit bytes directly. It is the **Text** tab inside each project window.

## Opening the hex view

Open a project, then select the **Text** tab in the project window (or **View ▸ Hex Editor**). The view shows the ROM as classic rows of 16 bytes: an address column on the left, the hex bytes in the middle, and a sidebar on the right that can show ASCII or per-byte bars.

## Navigating the hex dock

- Scroll with the mouse wheel or the scrollbar.
- A vertical **overview minimap** sits beside the scrollbar, showing the whole ROM's byte intensity at a glance; click or drag on it to jump to that region.
- The bytes are interpreted using the **Format** toolbar settings — data size (8/16/32-bit), byte order (LE/BE), sign and display format — so the same ROM can be read as bytes, words or longs.
- Adjust the font size with the Format toolbar spinner or **View ▸ Zoom In / Zoom Out** (`Ctrl++` / `Ctrl+-`).

## Address jumping

- **Find ▸ Find Address…** prompts for an address (hex or decimal) and scrolls to it.
- Selecting a map in the Map Selection panel, or clicking a map band in the 2D view, brings you to the corresponding bytes.
- Switching between the Text and 2D views carries your position across, so you stay on the same region.

## Pattern search

Address lookup is available directly from **Find ▸ Find Address…**. To locate content by bytes or find related ROMs, use **Find ▸ Find Similar Files…**. For comparing two ROMs byte-by-byte (rather than searching for a pattern), use the **Differences** panel described below.

## Highlighting map regions

Defined maps are drawn as coloured regions in the hex view, so you can see at a glance which bytes belong to which table. The colours come from the map highlight bands you can customise in **Settings ▸ Colors**. Addresses that carry a comment show a small ✎ marker in the offset gutter, and hovering shows the comment text. Add comments and markers from the **Find** menu (**Insert comment…**, **Insert marker**, **Next/Previous marker**).

## Byte diff between ROMs

There are two overlays for comparing:

- **Differences vs Original** (**View ▸ Differences vs Original**, `Ctrl+Shift+O`) highlights every cell that differs from the project's own original ROM, colouring it by how large the change is. This works in the hex, 2D and 3D views at once.
- The **Differences** panel (**View ▸ Differences**, `Ctrl+D`) compares two open projects byte-by-byte, listing each differing address with the A value, B value and delta, and can copy differences from one into the other. See [View Sync](14-view-sync.md) for aligning two ROMs.

## Editing raw bytes

You can edit bytes directly in the hex view:

- Click a byte and type hex digits to overwrite it.
- **Copy** (`Ctrl+C`) writes the selected bytes as space-separated hex; **Paste** (`Ctrl+V`) reads space/whitespace-separated hex bytes and overwrites forward from the caret.
- **Undo/redo** (`Ctrl+Z` / `Ctrl+Y`) is available, with a byte-level history of up to 100 steps.
- Right-click for the same value operations available elsewhere; all edits share the project's undo stack, so a hex edit and a 2D edit undo consistently.

!!! tip
    Direct byte editing is powerful but unforgiving — it bypasses map scaling. For calibration work, prefer the [Map Editor](10-map-editor.md) views, which show physical values and axes.
