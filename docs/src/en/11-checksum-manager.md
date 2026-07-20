# Checksum Manager

ECUs store one or more checksums that guard the integrity of the flash. When you change a map, those checksums no longer match, and the ECU will usually refuse the file. The Checksum Manager verifies and repairs the checksum for your ECU so your modified ROM is accepted.

!!! info "Pro feature"
    **Verify Checksum** and **Correct Checksum…** are available in Pro builds. Community builds do not include the checksum actions.

## Why checksums matter

A checksum is a value computed over a region of the ROM and stored back in the ROM. After editing, the stored value is stale. If you flash a ROM with a wrong checksum, most ECUs reject it (or run in a limp/default mode). Always verify, and correct if needed, before exporting a ROM for flashing.

## The Checksum Manager dialog

Both actions live on the **Project** menu and share a small **Select Checksum Algorithm** dialog:

1. Choose **Project ▸ Verify Checksum** or **Project ▸ Correct Checksum…**.
2. The dialog shows your project's **ECU type** and an **auto-detected algorithm** (romHEX 14 scans the ROM for embedded ECU identifiers such as "MED17", "EDC17", "EDC15P", "ME7." or "SIMOS" and validates that the algorithm's address ranges fit the ROM).
3. Confirm or change the selection in the **Select algorithm** dropdown, then click **OK**.

Built-in (cross-platform) algorithm families are marked "(built-in)" in the list.

## Built-in algorithms

romHEX 14 includes native, cross-platform implementations that work on Windows, macOS and Linux for these common families:

- **Bosch EDC15** (16-bit block complement sum)
- **Bosch ME7.x** (32-bit block-descriptor sum)
- **Bosch EDC16 / EDC16+** (block CRC32)
- **Bosch MED17 / EDC17** (multi-block CRC32, Tricore)
- **Bosch ME9 / MED9** (CRC32)
- **Siemens/Continental SIMOS** (size-dependent: sum for small ROMs, CRC32 for large)

If your ECU is one of these, verify and correct work on any platform with no extra setup.

## Checksum DLL bridge

Beyond the built-in families, romHEX 14 can drive a library of **148 ECU-specific checksum modules** (named `DEV001.dll`, `DEV002.dll`, …) covering a very wide range of Bosch, Siemens/Continental, Delphi, Denso, Marelli, Motorola and other ECUs across cars and trucks. These modules are **32-bit** and **Windows-only**.

Because romHEX 14 itself is 64-bit, it loads the 32-bit modules through a small helper process, **`checksumhelper.exe`**, which runs the DLL in a separate 32-bit subprocess and passes the ROM back and forth via temporary files. The helper is searched for in `ChecksumDLL/checksumhelper.exe`, next to the application executable, or one directory up.

!!! warning "Windows prerequisite: Visual C++ 2005 SP1 (x86) runtime"
    The 32-bit checksum modules link against the **Microsoft Visual C++ 2005 SP1 Redistributable (x86)** runtime. If it is not installed, the modules fail to load and romHEX 14 reports:

    > "The checksum DLL could not be loaded because a runtime dependency is missing. Please install the Microsoft Visual C++ 2005 SP1 Redistributable (x86) and try again."

    Install the **x86** (32-bit) redistributable — even on 64-bit Windows — and retry. Other DLL runtime dependencies (such as `gmp.dll`, `msvcr80.dll` and `mfc80.dll`) are expected to sit alongside the modules in the `ChecksumDLL` folder.

Other messages you may see from the bridge include "checksumhelper.exe not found" (the helper is missing), "The DLL is not a valid 32-bit library", "Checksum helper process timed out" and "Unknown ECU — no checksum DLL matched". On macOS and Linux, DLL-based ECUs are unavailable — a note reminds you that extended checksum support requires Windows, while the built-in families still work.

## Verifying a ROM

Run **Project ▸ Verify Checksum**, pick the algorithm and click **OK**:

- **Match** — the status bar shows "Checksum OK — *ECU* (*algorithm*)".
- **Mismatch** — a dialog reports "✗ Checksum mismatch" and advises using **Correct Checksum** before flashing.
- **Unsupported** — if no algorithm matches the ECU (or an ECU needs the Windows DLL you do not have), an information dialog explains that verification is not available for it.

## Recalculating before save

Run **Project ▸ Correct Checksum…** to repair the checksum before you export:

1. Choose the algorithm and click **OK**.
2. A confirmation dialog restates the ECU and algorithm and notes that the correction "modifies ROM data in memory (not saved until export)". Click **Yes**.
3. On success the status bar shows "Checksum corrected — *ECU* (*algorithm*)".

The correction is applied to the in-memory ROM and marks the project modified. It is written to disk only when you save the project or, for flashing, when you use **Export ROM…**. As a rule: edit your maps, **Correct Checksum**, then **Export ROM** — in that order.
