# A2L Import

The A2L import workflow is the most powerful way to bring an ECU into romHEX 14. An A2L supplies the names, addresses, dimensions and scaling for every map, so instead of raw byte regions you get a fully labelled, correctly-scaled calibration.

## What is an A2L file?

An A2L (ASAM MCD-2 MC) file is the standardised description that accompanies an ECU's software. It defines each calibratable object — its address, data type, size, axes and conversion formula — so tools know how to read the raw bytes as physical values. romHEX 14 parses the following from an A2L:

- **CHARACTERISTIC** objects — the maps, curves and single values;
- **AXIS_PTS** — shared axis breakpoint arrays;
- **RECORD_LAYOUT** — element size, sign and cell order;
- **COMPU_METHOD** — the conversion from raw to physical units;
- **GROUP** — the folder structure used to organise maps;
- **MOD_COMMON** — the default byte order (`MSB_LAST` means little-endian; otherwise big-endian).

The map types that are imported are **VALUE**, **CURVE**, **MAP** and **VAL_BLK**. ASCII/string characteristics are parsed but not added to the map list.

## Opening the A2L Import dialog

You must have a project open first (the A2L is applied to that project's ROM). If none is open, romHEX 14 asks you to create or open one.

1. Choose **Project ▸ Import A2L…**.
2. Pick the file in the **Import A2L File** dialog (filter `A2L Files (*.a2l)`). You can also drag a `.a2l` file onto the window.
3. A progress dialog titled **Importing A2L** parses the file — large A2Ls are handled on a dedicated high-stack worker so they parse reliably.

## Selecting the A2L and HEX files

There is **no separate HEX/binary picker** in the A2L flow. The A2L is imported onto the ROM you already loaded when you created the project. The base address is detected **from the A2L itself** — romHEX 14 tries a table of common ECU base addresses (Bosch MED17/EDC17/ME7, Continental/Simos, Renesas RH850, Denso, Marelli, Delphi and others) and progressively coarser alignments — so you do not have to enter it by hand.

!!! note "Match the A2L to the right ROM"
    An A2L only fits the exact firmware it was written for. If you import an A2L onto a different ROM, most map addresses will fall outside the binary. Import onto the matching original ROM first, then use **Link ROM to Project…** to carry the maps to a related file.

## The characteristics tree

After parsing, the **Import A2L – Select Maps** dialog appears. The header line reports how many characteristics were found and the detected base address. Maps are shown as a **tree** with tri-state checkboxes and five columns:

| Column | Contents |
|---|---|
| **Name** | Characteristic name |
| **Type** | MAP, CURVE, VALUE or VAL_BLK |
| **Address** | ROM address in hexadecimal |
| **Size** | Size in bytes |
| **Description** | Long identifier (also shown as a tooltip) |

## Filtering and grouping

- **Grouping** — if the A2L defines GROUP blocks, they appear as bold folders (with sub-groups nested inside). Maps with no group go under **(Ungrouped)**; if the A2L had no groups at all, everything sits under a single **All Maps** folder. Ticking a folder selects everything inside it.
- **Filtering** — type in the **Filter** box to show only rows whose name or description matches. Filtering only hides rows; it does not change what is checked, so a filter cannot accidentally deselect maps.
- **Bulk selection** — **Select All**, **Select None** and **Invert** act on every map, including rows currently hidden by the filter.

## Selecting maps to import

Tick the maps (or whole groups) you want. The counter shows "*N* of *M* selected" and the confirm button reads **Import Selected (*N*)**. It is disabled while nothing is checked. Click it to import.

## Address and scaling preview

The selection tree shows each map's address (hex) and size (bytes). Physical scaling is applied later, when you open a map, using its COMPU_METHOD:

- **Linear** conversions (`physical = a·raw + b`) and **rational-function** conversions are fully decoded.
- Tabular/verbal conversions (COMPU_TAB / COMPU_VTAB) are treated as identity — the raw value is shown.
- The A2L's format string drives display precision, and the unit is stored for axis and value labels.

There is no live physical-value preview inside the import dialog itself; values are computed when the map is opened in the editor.

## Confirming the import

When you click **Import Selected**, every chosen map is validated against the ROM and the **A2L Import Results** dialog appears:

- **Total maps in A2L** — how many you selected.
- **Valid (in ROM)** — how many landed inside the ROM, with a percentage (green).
- **Out of bounds** — how many pointed outside the ROM, with a percentage (red). A map is out of bounds if its address is below the base or if it runs past the end of the ROM.
- A per-type breakdown (**MAPs / CURVEs / VALUEs**).
- A **Compatibility** score (0–100) weighing the EPK identifier match, axis-count agreement and map-data smoothness.

If the A2L matches, click **OK** and the maps are added. If most maps are out of bounds, the dialog warns that the A2L was made for different firmware and offers **Import *N* valid maps anyway** or **Cancel import**, along with the recommendation to import into the matching ROM and use Link ROM.

## Troubleshooting A2L issues

- **"Cannot open A2L file"** — the path is unreadable; check the file exists and is not locked.
- **Most maps out of bounds / low compatibility score** — the A2L does not match this ROM. Import it onto the correct original ROM, then link.
- **A map is missing** — ASCII/string characteristics are intentionally excluded, and any block whose header is malformed is skipped rather than crashing the parse.
- **Wrong base address** — the importer auto-detects the base; if values look shifted, verify you are using the ROM that belongs to this A2L.
- **Import cancelled** — closing the selection dialog leaves the project untouched.
