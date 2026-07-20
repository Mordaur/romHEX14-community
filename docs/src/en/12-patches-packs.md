# Patches & Packs

romHEX 14 can package your work so it can be applied to another ROM or shared with someone else. There are two formats for this — **patches** (`.rxpatch`) and **map packs** (`.rxpack`) — plus a simple CSV map-list interchange. This chapter explains when to use each and how.

## Patch vs Pack

| | Patch (`.rxpatch`) | Map Pack (`.rxpack`) |
|---|---|---|
| **Stores** | The *difference* between an original and a modified ROM (changed cells only, plus optional raw-byte changes outside maps) | *Complete* snapshots of the selected maps (full cell data, axes and scaling) |
| **Best for** | Reproducing a specific tune on the same base ROM | Transferring whole maps into a different project |
| **On apply** | Writes only the changed cells, verifying each against the reference | Overwrites the target maps with the packed values |

A patch is a precise, minimal record of "what changed"; a pack is a self-contained set of finished maps.

## Creating a patch from a diff

A patch is built from the difference between a reference ROM and a modified one. Each changed map records its geometry, a CRC of the source bytes (for integrity) and every changed cell as an original→new pair. Optionally it also captures raw byte changes that fall outside map regions (such as checksum/CRC tables) so the tune can be reproduced exactly.

You work with patches through the **Patch Script Editor** (**Project ▸ Open Patch Script…**). Use **Save .rxpatch…** to write a patch out; the file filter is `Patch scripts (*.rxpatch)`.

## Applying a patch

1. Open **Project ▸ Open Patch Script…** to launch the **Patch Script Editor**.
2. Click **Open .rxpatch…** and choose the patch. The left table lists each map with its changed-cell count and data size; the right pane shows the editable raw JSON.
3. Apply it with **Apply to current ROM**, **Apply to linked ROM…** or **Apply to ROM file…**.

When applying, romHEX 14 matches each map by name and address, verifies the original values (a CRC mismatch is a warning, not a hard failure), and falls back gracefully if a map has moved. Large patches ask you to type **APPLY** to confirm, and a version snapshot ("Before patch: …") is offered so you can revert. A **Patch Results** dialog then summarises how many maps applied cleanly, with warnings, or failed.

!!! warning "Raw checksum bytes are ROM-specific"
    If a patch carries raw byte changes outside map regions (ECU-specific checksums/CRC data), romHEX 14 warns before applying them: those bytes are only correct for the exact same ECU variant and base ROM. Applying them to a different base ROM produces wrong checksums. You can choose to apply the maps only.

## Building a Map Pack

A map pack stores finished maps, including their axis values and scaling, so it needs no A2L on the receiving end.

1. From a comparison (the changed maps of a ROM) or an explicit selection, open the **Map Pack** dialog.
2. Tick the maps to include (**Select all** / **Select none**), set a **Pack label**, and click **Save .rxpack…** (filter `Map packs (*.rxpack)`).

To apply one: **Project ▸ Import Map Pack…**, choose the `.rxpack`, tick the maps you want, and click **Apply selected to ROM**. romHEX 14 resolves each map's target (by explicit offset, by name, or by the address stored in the pack), byte-swaps cells if the pack's byte order differs, and warns about dimension mismatches or out-of-bounds maps. As with patches, large applies require typing **APPLY**, a version snapshot is offered, and a **Map Pack Applied** summary reports the result.

## CSV map list import/export

romHEX 14 supports two different CSV features — don't confuse them:

**Map-list interchange (for map packs)** — a lightweight `Address;Name;Size` list of map *definitions* with no cell data.

- **Import** with **Project ▸ Import Map List (CSV)…**. The header must contain **Address**, **Name** and **Size** columns; addresses may use `$` or `0x` prefixes, and Size is written as `cols×rows`. Importing registers the map definitions in your project without touching ROM bytes (they arrive via the "Add selected to project" step of the Map Pack dialog). The Map Pack **Save** dialog can also export this CSV.

**Map-list report (for spreadsheets)** — a full one-way export of your project's maps for Excel or Sheets.

- Use **Project ▸ Export map list as CSV…** or **Export map list as JSON…**. The CSV includes Name, Address, Size, Type, Cols, Rows, DataSize, Signed, ByteOrder, X-axis, Y-axis, Min/Max/Mean values, modified-cell count and your notes. This report is for review, not for re-import as a pack.

## Sharing tunes safely

- Prefer a **patch** when the recipient has the exact same base ROM — it is minimal and verifies every cell it touches.
- Prefer a **map pack** when transferring maps into a different (but compatible) project — it is self-contained.
- Be careful with **raw checksum bytes**: only include or apply them for the identical ECU variant and base ROM; otherwise apply maps only and re-run **Correct Checksum** on the target.
- Applying either format offers a version snapshot first, so the recipient can always revert. Encourage recipients to verify the checksum before flashing.

## Patch editor reference

The **Patch Script Editor** (920×680) contains:

- a **Patch metadata** group — an editable **Label** and read-only **Source** / **Target** fields;
- a **map table** with **Map**, **Changed cells** and **Data size** columns;
- an editable **raw JSON** pane (changes there are applied when you apply the patch);
- a status line summarising map and cell counts and the creation date;
- buttons: **Open .rxpatch…**, **Save .rxpatch…**, **Apply to current ROM**, **Apply to linked ROM…**, **Apply to ROM file…** and **Close**.

The two "apply to project" buttons are disabled when no project is open.
