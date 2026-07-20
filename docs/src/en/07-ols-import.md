# OLS Import

Import existing OLS-format projects (`.ols` files) directly into romHEX 14, including the ROM binary, map definitions, axes and multiple firmware versions. An OLS project is a complete container — unlike an A2L or a map pack, it carries the ROM data itself — so importing one creates a brand-new romHEX 14 project.

## OLS file format primer

The OLS format is the project container used by common ECU tuning tools. A single `.ols` file bundles:

- **project metadata** — make, model, engine, ECU name, hardware/software numbers, engine code, transmission and more;
- one or more **ROM versions** (a project can hold several firmware revisions or savepoints);
- **map records** — name, description, type, address, dimensions, cell data type and linear scaling;
- **axis definitions** — the X (column) and Y (row) breakpoints, with their own scaling and units.

romHEX 14 reads both modern and legacy OLS layouts (the modern format is detected by its internal format-version number and segment markers).

## Opening the OLS Import dialog

1. Choose **Project ▸ Import OLS…**, or click the **OLS** button on the toolbar.
2. Select the file (filter `*.ols` and `*.kp`). romHEX 14 parses the container and builds a new project.

There is no version-picker step — see below.

## ROM extraction

The importer locates the flash segments inside the OLS file (using the format's segment descriptors or, for older files, the per-version directory) and assembles each version into one contiguous ROM, filling any gaps with `0xFF`.

For a **multi-version** project, romHEX 14 does not prompt you to choose one version. Instead it builds a single project where:

- **Version 0** becomes the main, editable ROM;
- the remaining versions are stored as **version snapshots** inside the same project.

You reach the extra versions afterwards through the **Versions** node in the project tree and the **Open Version in New Window** submenu.

## Map records and metadata

Each map record is imported with its name, description, type (classified as VALUE, CURVE or MAP), address, X/Y dimensions, cell size and linear scaling (factor, offset and unit). Project metadata from the OLS file populates the new project's brand, model, ECU, hardware/software numbers, engine code and notes. After import, romHEX 14 also runs a light ECU auto-detect to fill any blank hardware/software fields without overwriting values already present.

The status bar confirms the result, for example "Imported project: *name* — *N* maps, *M* extra version(s)".

## Axes and scaling

X (column) and Y (row) axes are imported with their input name, unit, linear scaling, breakpoint address, count and data type. romHEX 14 reads the actual breakpoint values out of the assembled ROM (decoding each per its data type and byte order) so axes display real physical numbers, not just indices.

## Byte order handling

OLS files mix byte orders, and romHEX 14 resolves this at several levels:

- The container headers are read little-endian.
- The project's global byte-order toggle is set to little-endian on import (you will see the **LE/BE** toggle on the Format toolbar reflect this).
- Crucially, **byte order is resolved per map and per axis** from each record's data-type field — so a single ROM can legitimately contain both big-endian and little-endian maps, and each is decoded correctly.

## Importing into a project

Because an OLS import produces a full project, it opens directly as a new project window; there is no "import into the current project" step (that model is used by A2L and KP instead). If nothing could be extracted, an **Import Error** dialog reports "No versions found".

To go the other way — writing a romHEX 14 project back out as an OLS file — use **Project ▸ Export OLS Project (.ols)…**.

## Compatibility notes

- Both modern and older OLS layouts are supported; the format version is read from the file header.
- The map-record parser understands a range of schema versions, including universal base-address translation for newer Tricore-based ECUs (for example MED17.7-class firmware).
- If imported values look wrong, confirm the file opened as the expected version and check the per-map data organisation in [Map Properties](10-map-editor.md#map-properties); because byte order is per-map, an individual map can be re-specified there if a record was ambiguous.
- To import only **map labels** (names/addresses without a ROM) on top of a project you already have, use a [KP map pack](08-kp-import.md) instead.
