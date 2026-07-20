# Introduction

romHEX 14 is a professional ECU calibration suite built on Qt 6 with native support for A2L, Intel HEX, OLS-format (`.ols`) and KP map-pack files. This chapter introduces the core concepts you will encounter throughout the rest of the manual.

## What is ECU calibration?

An engine control unit (ECU) runs fixed program code, but its *behaviour* is governed by thousands of adjustable numbers stored in its flash memory — how much boost to target, how much fuel to inject, when to fire the spark, what limits to enforce. These numbers are organised into tables and curves. **Calibration** (or "tuning") is the practice of reading those numbers out of a ROM dump, understanding what each one does, adjusting them for a goal (more power, different hardware, emissions-equipment changes), and writing a corrected file back. romHEX 14 is the workbench for that process: it turns a raw binary into labelled, scaled, editable maps and helps you package and verify the result.

## Why romHEX 14?

- **Everything in one project file** — your ROM, its original snapshot, all map definitions, your edit history and metadata live in a single portable `.rx14proj`.
- **Every common source format** — import A2L descriptions, complete OLS projects, or KP map packs, or let romHEX 14 auto-detect maps in a raw dump.
- **Three linked views** — inspect and edit the same data as a hex dump, a 2D waveform, or a 3D surface with an operating-point simulation.
- **Safe by design** — original values are always retained, edits are undoable, Tuning Branches let you experiment freely, and checksum tools help ensure a flashable result.
- **Cross-platform** — the same application on Windows, macOS and Linux.
- **Optional AI assistance** — in Pro builds, an AI assistant can analyse and edit maps and label characteristics.

## Key concepts: ROM, map, axis, characteristic

- **ROM** — the binary image of the ECU's flash memory (the dump you read from the car). romHEX 14 keeps both your working copy and the untouched original.
- **Map** — a table of calibration values. A **VALUE** is a single number, a **CURVE** is a 1D table (one axis), and a **MAP** is a 2D table (two axes).
- **Axis** — the breakpoints that label a table's rows and columns (for example engine speed across the top, load down the side). Axes have their own scaling and units.
- **Characteristic** — the A2L term for a calibratable object; each characteristic becomes a map in romHEX 14.
- **Scaling (conversion)** — the formula that turns the raw stored bytes into a physical value (bar, °, mg/stroke). romHEX 14 shows physical values while editing and writes the raw bytes for you.
- **Base address / offset** — where a map lives in the ROM. The base address ties an A2L's addresses to positions in the binary.

## Supported file formats

| Format | Extension | Role |
|---|---|---|
| romHEX 14 project | `.rx14proj` | Native project container (ROM + maps + history) |
| A2L description | `.a2l` | Map definitions and scaling to import onto a ROM |
| ROM binary / Intel HEX | `.bin`, `.rom`, `.hex`, … | The flash image itself |
| OLS project | `.ols` | Complete project (ROM + maps + versions) to import/export |
| KP map pack | `.kp` | Map labels only, applied to an existing ROM |
| Patch | `.rxpatch` | The difference between two ROMs, for reproducing a tune |
| Map pack | `.rxpack` | Complete map snapshots, for transferring maps |

See [Appendix B — File Formats](appendix-b-formats.md) for technical detail.

## Workflow overview

A typical session looks like this:

1. **Create a project** from a ROM dump ([Projects](05-projects.md)).
2. **Get maps** — import an [A2L](06-a2l-import.md), an [OLS project](07-ols-import.md) or a [KP pack](08-kp-import.md), or let auto-detect find them.
3. **Edit** in the 2D/3D [Map Editor](10-map-editor.md), using value operations, interpolation and smoothing.
4. **Verify integrity** with the [Checksum Manager](11-checksum-manager.md) (Pro).
5. **Export the ROM** for flashing, or **package** the tune as a [patch or map pack](12-patches-packs.md).
