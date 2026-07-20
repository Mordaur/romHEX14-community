# Projects

Everything about projects: creating them, importing existing work, organising multi-ROM projects, exporting and sharing. A project is the container that binds a ROM, its map definitions, your edits and your history together in one portable `.rx14proj` file.

## Creating a new project

Choose **Project ▸ New Project…** (`Ctrl+N`) or click **New** on the toolbar to open the **New Project** dialog.

1. Click **Browse…** in the **ROM File** group and select your binary (`*.bin`, `*.rom`, `*.hex`, `*.dat`, `*.ori`, `*.mod`, `*.full`, `*.mpc`).
2. When a ROM is picked, romHEX 14 auto-fills what it can:
    - the **Model** is guessed from the filename (stripping suffixes like `_ori`, `_stock`, `_mod`, `_backup`);
    - the **ECU Type** is detected from the ROM bytes;
    - the **SW Number** is scanned from the first 64 KB (looking for `SW:` / `HW:` / `Cal:` markers and Bosch-style part numbers).
3. Fill in or adjust the **Vehicle / ECU Information** fields: **Brand**, **Model**, **Engine**, **ECU Type**, **SW Number**, **Transmission**, **Displacement**, **Year** and free-form **Notes**. The Brand/Model/Engine/Transmission fields are combo boxes backed by a built-in vehicle database and cascade as you choose.
4. Click **Create Project**. The button is disabled until a ROM is selected. The project name defaults to "Brand Model", or the ROM filename if you left those blank.

## Project structure on disk

A saved project is a single file with the extension **`.rx14proj`**. It is a self-contained binary container (not a ZIP), and everything the project needs travels inside it:

- the current (edited) ROM and a snapshot of the **original** unedited ROM;
- A2L-derived map definitions, plus the raw A2L file embedded verbatim for portability;
- auto-detected maps (when no A2L is present);
- A2L groups, starred maps, linked ROMs and saved versions;
- all metadata (vehicle, client, ECU, engine) and your notes, tuning logbook and dyno history.

Saves are **atomic** — romHEX 14 writes to a temporary file and renames it over the target, so a crash mid-save cannot corrupt your project.

A few sidecar files live next to the `.rx14proj`, sharing its base name:

| File | Purpose |
|---|---|
| `<name>.savepoints.json` | Tuning Branches (see below) |
| `<name>.comments.json` | Per-address comments and markers |
| `<name>.legacy.bak` | Backup created when migrating an older project |

Projects created by earlier versions in the old JSON format are detected and migrated automatically the first time you open them; the original is kept as `<name>.legacy.bak`.

## Project properties dialog

Open **Project ▸ Project Info…** (also on the **Miscellaneous** menu) to view and edit the full record for the project. The **Project Properties** dialog is a scrollable, multi-column form grouped into sections:

- **Client** — Name, Customer nr., Licence, VIN.
- **Vehicle** — Type, Producer, Series, Build, Model, Characteristic, Model year.
- **ECU** — Use, Producer, Build, ECU numbers, Software and version, Processor and 8-bit sum (read-only; auto-populated each time the dialog opens).
- **Engine** — Producer, Motorcode, Type, Displacement, Output (PS/kW), Max. torque, Emission class, Transmission.
- **Project** — ROM file, saved file path, folder, created/changed timestamps, software size, project type (in development / released / archived / for sale / prototype) and map language.
- **Notes** — free text.

Click **Finish** to keep your changes or **Cancel** to discard them.

## Project Manager

The **Project Manager** (**Project ▸ Project Manager…**, `Ctrl+M`) is your library of projects. It opens on the **Local Projects** tab:

- A **Filter** box searches by name, brand or ECU; a **Refresh** button reloads the list from disk.
- The table shows **Project**, **Vehicle**, **ECU Type**, **Client**, **Last modified** and **File path**. Projects that embed an A2L carry a small green **A2L** badge; projects whose file has gone missing are dimmed.
- Buttons let you create (**New Project…**), **Open**, **Rename…**, **Remove from list** or **Delete file…**. *Remove from list* only forgets the project — it never deletes the file on disk. *Delete file…* permanently deletes and cannot be undone.
- Right-click a row for the same actions plus **Show in Explorer**.

A second tab, **WOLS Catalog**, browses a catalog database of OLS-format projects so you can locate and open a `.ols` file by make, model, engine or ECU.

## Multi-ROM projects

A single project can hold more than one ROM in three complementary ways:

- **Versions** — snapshots of the whole ROM. Create one with **Project ▸ Save Version Snapshot…**, or bring an external file in with **Import ROM as Version…**. Saved versions appear under the **Versions** node in the project tree and can be opened read-only via **Open Version in New Window**.
- **Linked ROMs** — use **Project ▸ Link ROM to Project…** to attach another raw ROM. romHEX 14 auto-locates every map from the reference ROM inside the linked file and opens it side by side, with cursor sync enabled so the two line up.
- **Sub-projects** — importing a multi-version OLS project creates one parent shell plus a child sub-project for each embedded version, each with its own ROM and maps.

Use **Project ▸ Compare ROM / Version…** (Pro) to diff the current ROM against a linked ROM or a saved version.

## Tuning Branches (savepoints)

**Tuning Branches** are named snapshots of the current ROM state that you can switch between instantly — think of them as labelled checkpoints for "boost +0.2 trial" or "stage 2 experiment". They are available in every edition.

Open the dock with **View ▸ Tuning Branches** (`Ctrl+B`).

- Type a name in the **Branch name** field and click **Save current** to snapshot the ROM as it stands.
- Select a branch and click **Switch to** (or double-click it) to restore the ROM to that state. The active branch is marked with a green ● dot.
- **Rename** or **Delete** the selected branch with the corresponding buttons.

Each branch is stored as a compact delta against the project's original ROM in the sidecar `<name>.savepoints.json`. Branches are all rooted at the original data (a flat list, not a nested tree), and a branch made against a different ROM size cannot be switched in.

## Importing and exporting

- **Import maps** into an existing project from an [A2L](06-a2l-import.md), a [KP map pack](08-kp-import.md), or a [Map Pack or CSV map list](12-patches-packs.md).
- **Open a complete OLS project** as a new project via **Project ▸ Import OLS…** (see [OLS Import](07-ols-import.md)).
- **Export the ROM** with **Export ROM…** (`Ctrl+E`) as a `.bin`/`.rom`.
- **Export the project** in OLS format with **Export OLS Project (.ols)…**.
- **Export the map list** for spreadsheets with **Export map list as CSV…** or **Export map list as JSON…**.

## Backups and recovery

- **Auto-save** is on by default (**Miscellaneous ▸ Auto Save ▸ After Delay** — 5 seconds after your last edit). Other modes are **Off**, **On Focus Change** and **On Window Deactivate**. Auto-save writes the real `.rx14proj` atomically; it does not scatter `.autosave` files.
- The status bar shows the save state ("● Modified" / "✓ Saved *N*s ago") so you always know whether your work is on disk.
- When an old-format project is migrated, the original is preserved as `<name>.legacy.bak`; if migration fails, that backup is restored automatically.
- Because a project embeds its ROM, its original snapshot and its history, copying the single `.rx14proj` file is a complete backup.
