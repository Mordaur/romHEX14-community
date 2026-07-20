# Appendix B — File Formats

Technical notes on the file formats romHEX 14 reads and writes. "Import" means
romHEX 14 can read the format; "Export" means it can write it.

## A2L / ASAP2 (.a2l)

**Import.** The ASAM MCD-2 MC description language. An A2L describes an ECU's
calibration objects — `CHARACTERISTIC` maps and curves, `AXIS_PTS` axes,
`COMPU_METHOD` scaling, and `GROUP` folders — but contains no ROM data itself,
so you import an A2L on top of a project that already has a ROM. romHEX 14 reads
characteristic names, addresses, dimensions, data types, linear and rational
scaling, and reproduces the `GROUP` / `SUB_GROUP` hierarchy as folders in the
map tree. See [A2L Import](06-a2l-import.md).

## Intel HEX (.hex) and Motorola S-record (.s19 / .srec)

**Import and export.** Text-based ROM containers that store address/data
records with checksums. romHEX 14 auto-detects either format when you open a
file and can export the current ROM back to Intel HEX or S-record from
**Project → Export ROM**.

## Raw binary (.bin / .rom / .ori / .mpc)

**Import and export.** A flat dump of the ROM with no framing. This is the most
common working format; the whole file is the image.

## OLS project (.ols)

**Import and export.** A multi-version project container (a compressed archive
of an internal object store). romHEX 14 reads the embedded ROM segments,
per-version snapshots, map definitions with axes and scaling, and project
metadata, presenting each version as a sub-project. Export writes a compatible
project back out for round-tripping. See [OLS Import](07-ols-import.md).

## KP map pack (.kp)

**Import.** A compressed map-definition pack (a ZIP whose `intern` entry holds
length-prefixed records). romHEX 14 reads three record-layout generations,
including the OLS 5.x "schema 750" layout, decoding each map's address,
dimensions, cell size, scaling, and X/Y axis blocks (address, element size,
name, unit, and linear scaling). The friendly map name is used as the
description; the internal id slug is preserved as a side property. See
[KP Import](08-kp-import.md).

## Map list (.csv / .json)

**Import and export.** A tabular list of map definitions (name, address, type,
dimensions, scaling) with no ROM bytes. Use it to move a definition set between
projects or to review it in a spreadsheet. CSV import can apply the definitions
to the current ROM.

## romHEX patch (.rxpatch)

**Import and export.** A record of byte-level edits (address + original + new
bytes) that can be applied to a matching ROM. Patches capture *changes*, not a
whole image, so they are small and can be shared and re-applied. See
[Patches & Packs](12-patches-packs.md).

## romHEX pack (.rxpack)

**Import and export.** A JSON map pack bundling map definitions (and optionally
their values) for distribution. Unlike a patch, a pack describes maps rather
than raw byte deltas.

## romHEX project (.rx14proj)

**Import and export.** The native project file: a container holding the ROM
image(s), all versions, map definitions, annotations, and metadata. This is the
format to use when you want to save and resume your full working state.

## Tuning report (.html)

**Export only.** A self-contained HTML report comparing the current ROM against
its original snapshot, listing each changed map with its physical value range
before and after. Open it in any browser or print it to PDF. See
[Patches & Packs](12-patches-packs.md).
