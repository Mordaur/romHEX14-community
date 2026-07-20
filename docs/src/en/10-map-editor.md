# Map Editor

The map editor is the heart of romHEX 14. Every open project window offers three views of your data — **Text** (hex), **2d** (waveform) and **3d** (surface + simulation) — selectable from the tabs or the **View** menu (**Hex Editor**, **Waveform**, **3D Map**). This chapter covers editing in the 2D and 3D views; the raw byte view is covered in [Hex Overview](09-hex-overview.md).

## 2D map view

The 2D view is a horizontal waveform explorer. Every cell in the ROM (read at the current data size and byte order) is plotted as a point or line, and map regions are highlighted as coloured bands. A top control bar lets you set the **Size** (8-bit / 16-bit / 32-bit), the byte order (**Big Endian / Little Endian**) and a **Zoom** slider; the readout shows values-per-pixel when zoomed out and pixels-per-value when zoomed in. Higher values render brighter (a blue-to-cyan heat ramp), so the shape of a map is visible at a glance. Hovering shows the address, decimal value and hex value under the cursor.

## 3D map view

Selecting a map and switching to the **3d** view renders it as a colour-shaded surface (a map needs to be at least 2×2). Controls:

- **Rotate** — left-drag (horizontal spins, vertical tilts).
- **Zoom** — mouse wheel.
- **Right-click** for a context menu with an **Edit map** submenu (the same value operations as the Selection menu, applied to the whole map), **Reset view**, and a **Wireframe** toggle.

The 3D view also includes an **operating-point simulation**: two sliders labelled with the map's axis units (for example **RPM** and **Load**) move a crosshair across the surface, and a **RPM slice** and **Load slice** cut through it so you can read the interpolated output at any point. A top-left **mini heatmap** shows every cell as a coloured square. The surface crosshair and the mini heatmap cursor stay in sync: click or drag on either, or use the arrow keys, and both update together. The status badge reports the selected cell, its address, raw and scaled value, and the current X/Y inputs.

## Axes and scaling

Axis breakpoints are read from the ROM at each axis's own address and displayed as physical values (scaled by the axis conversion) along the front (X) and left (Y) edges of the 3D view, and used to label the simulation sliders. When a map has no defined axis, cell indices (0, 1, 2 …) are shown instead. To change how an axis is read — its address, data size, byte order, sign or scaling — use the **X-Axis** and **Y-Axis** tabs of the [Map Properties](#map-properties) dialog.

## Selecting cells

In the 2D view:

- **Left-drag** selects a byte range (a dashed box shows the size and address range).
- **Single click** on a map region opens that map.
- **Middle-drag** pans; the mouse wheel scrolls.
- **Ctrl+A** selects the whole ROM; **Esc** clears the selection.
- Arrow keys, **Home/End** and **PageUp/PageDown** move and extend the selection; hold **Shift** to extend.

The 3D view has no per-cell selection — its edit operations act on the entire current map.

## Editing values

With a selection active you can edit in several ways:

- **Type an exact value** — press `=` (Change absolute) and enter the value to set every selected cell to it.
- **Add or scale** — press `%` (Change relative) and enter a delta (`+5`, `-3`), a factor (`*1.10`) or a percentage (`+5%`).
- **Nudge** — `+` and `-` change every selected cell by one.
- **Draw** — hold **Shift** and drag the trace up or down to set values freehand.
- **Delete** sets the selection to 0.

All of these, plus the ones below, are also on the **Selection** menu and the right-click menu. Each operation is a single undo step.

## Interpolation tools

The **Selection** menu (and the 2D right-click menu) provides the shaping tools tuners rely on:

- **Interpolate** — keeps the first and last selected cells fixed and fills a smooth linear ramp between them.
- **Smooth** — replaces each cell with a moving average of its neighbours (a ±3-cell window), rounding off spikes.
- **Flatten (set to mean)** — sets every selected cell to the average of the selection.
- **Round / limit values…** — rounds to a chosen multiple and clamps to a minimum/maximum.
- **Change by slider…** — drag a slider to set a value with a live preview before committing.
- **Restore original value** (`F11`) — reverts the selected cells to the original ROM bytes.
- **Again** (`F4`) — re-applies your last operation to the current selection without re-prompting.

## Copy, paste and paste-special

- **Copy** (`Ctrl+C`) in the 2D view writes the selected values as comma-separated decimal numbers — ready to paste into a spreadsheet.
- **Paste** (`Ctrl+V`) reads numbers from the clipboard (separated by commas or whitespace) and writes them into the selection, starting at its first cell. The format is auto-detected, so there is no separate paste-special dialog.
- The hex view copies and pastes space-separated hex bytes instead (see [Hex Overview](09-hex-overview.md)).
- Additional per-map tools live on the **Tools** tab of Map Properties: **Export map data to CSV…** and **Copy raw values to clipboard**.

## Undo / redo history

Each project window keeps one undo stack shared across its hex, 2D and 3D edits, so you can undo consistently regardless of which view made the change. Use `Ctrl+Z` to undo and `Ctrl+Y` (or `Ctrl+Shift+Z`) to redo. The hex view additionally keeps its own byte-level history of up to 100 steps.

## Map properties

Right-click a map (or open it and use the properties command) to open **Properties of… *map name***, which controls exactly how the bytes are interpreted. It has five tabs:

- **Map** — Name, Description, Unit, **Start address** (with a *From hexdump cursor* button), **Type** (MAP / CURVE / VALUE / VAL_BLK), **Columns × rows**, **Data organization** (8/16/32-bit with LoHi = little-endian or HiLo = big-endian, plus a *Skip bytes* offset), **Number format**, the **Sign / Difference / Original values / Percent** flags, a **Factor, offset** group (physical = factor × raw + offset) and **Precision** (decimals).
- **X-Axis** / **Y-Axis** — the same address, data organization, sign, scaling and precision controls for each axis, plus a **Search axis…** helper.
- **Comment** — free-text notes for the map.
- **Tools** — CSV export, copy raw values and axis search.

Click **OK** to apply.

## Creating maps manually

When you spot a table in the hex or 2D view that has no definition, you can define it yourself:

1. Select the byte range in the 2D or hex view.
2. Open the **Create Map** dialog (from the selection's right-click "Selection → Map" action).
3. Choose a **template** or set **Columns × Rows** directly, give it a **Name** (blank auto-names it `UserMap_0x<address>`), confirm the **Start address**, **Cell size** (8/16/32-bit) and **Data type** (Signed/Unsigned). A live preview shows the total cell and byte count and how it compares to your selection.
4. Click **Create Map**. Single-row definitions are stored as CURVE, larger ones as MAP, and the new map appears in the Map Selection panel.

## AI-assisted editing

In Pro builds with an AI provider configured, the AI Assistant can read, analyse and edit maps on your behalf — for example scaling, smoothing or zeroing a table you describe in plain language. Every AI edit is confirmed and snapshotted so it can be reverted. See [AI Functions](13-ai-functions.md).
