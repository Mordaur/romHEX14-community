# KP Import

A KP file is a map pack — a bag of map labels (names, types, addresses, dimensions and scaling) with **no ROM binary**. Importing one applies those labels on top of the project you already have open, in the same way an A2L does. Use it to add a known map set to a ROM you dumped yourself.

## What is a KP file?

A `.kp` file is the map-definition pack produced by common ECU tuning tools. It contains the description of each map — name, type, address, columns/rows, scaling and axes — but not the calibration data itself. Because there is no ROM inside, a KP is always applied to an existing project's ROM.

## Supported KP versions

romHEX 14 reads three record layouts, covering the main tool generations:

- **Compact / newer records** — the current format.
- **OLS 5.x records** ("schema 750", written by the "OLS 5.0 (Windows)" generation) — parsed with full X/Y axis sub-blocks and an internal id string per map.
- **Legacy 4.x records** — older length-prefixed, NUL-terminated map records.

Internally the payload is a small ZIP archive; both stored and deflate-compressed entries are supported.

## Opening the KP Import dialog

1. Open a project with ROM data first. If none is open, romHEX 14 tells you to "Open a project with ROM data first" — KP packs are added on top of an existing project.
2. Choose **Project ▸ Import KP…** (or the **KP** toolbar button).
3. Pick the file (filter `KP map packs (*.kp)`).

!!! note "Direct, non-interactive import"
    In the current build, importing a `.kp` applies its maps directly to the open project — there is no separate map-selection dialog. The result is reported in the status bar. To review and pick individual maps before applying, use a [Map Pack (`.rxpack`) or CSV map list](12-patches-packs.md) instead, which does present a selection dialog.

## Map preview

After parsing, romHEX 14 resolves each map's address against your ROM (scoring candidate base/offset layouts, so Tricore-style `0x80xxxxxx` addresses and flat file offsets are both handled), de-duplicates against maps already in the project, and files the new maps into folders grouped by name prefix. Newly added maps then appear in the **Map Selection** panel on the left, where you can preview them in the 2D, 3D and hex views like any other map.

## Selecting maps to import

All maps that resolve inside your ROM and are not already present are added automatically. Duplicates (matched by name and address) are skipped. The status bar summarises the outcome, for example "Imported *N* maps from *file*" (with "(*N* already present, skipped)" when some were duplicates). If the pack contains nothing usable, you will see "No maps found in this .kp file."

## Version-specific quirks

- **OLS 5.x (schema 750)** packs include real X/Y axis blocks (columns first, then rows) with per-axis address, element size and scaling, plus an internal id slug stored alongside the map.
- **Legacy 4.x** packs do not always store dimensions explicitly; romHEX 14 recovers columns × rows by scanning for a pair whose product matches the cell count.
- **Compact** packs derive dimensions from header hints, numbers embedded in the map name, or a near-square fallback.
- If the number of decoded records does not match the pack's declared map count, a warning is logged — check that the pack matches the ECU family of your ROM.
