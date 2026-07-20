# Installation

This chapter covers installing romHEX 14 on Windows, macOS and Linux, plus first-launch configuration. romHEX 14 is a Qt 6 desktop application distributed as a native package for each platform.

## System requirements

- A 64-bit Windows, macOS or Linux desktop.
- A few hundred MB of free disk space; more if you keep many projects (each project embeds its ROM).
- For the AI features, an internet connection and an API key (or a local model endpoint). AI *tuning functions* and *map translation* also require a Pro account.
- **Windows only, for extended checksums:** the 32-bit checksum modules require the **Microsoft Visual C++ 2005 SP1 Redistributable (x86)** — see [Checksum Manager](11-checksum-manager.md).

## Windows installation

1. Download the Windows build from your romHEX 14 account or the distribution you were given.
2. Run the installer (or unzip the portable build) and launch the application.
3. If you plan to use the ECU-specific checksum modules, install the **Microsoft Visual C++ 2005 SP1 Redistributable (x86)** — the 32-bit (x86) version, even on 64-bit Windows — and make sure the `ChecksumDLL` folder and `checksumhelper.exe` are present alongside the executable.

## macOS installation

1. Download the macOS disk image (`.dmg`).
2. Open it and drag **romHEX 14** to your Applications folder.
3. Launch it from Applications. The built-in checksum families work on macOS; the extended ECU-specific modules are Windows-only.

## Linux installation

1. Obtain the Linux build for your distribution.
2. Ensure the Qt 6 runtime libraries it depends on are available (most desktop distributions provide these, or they are bundled with the package).
3. Launch the application. As on macOS, the built-in checksum families are available; the extended DLL-based modules are Windows-only.

## First launch

On first run romHEX 14 opens with an empty workspace and the status bar prompt "Ready — Open a ROM file or project to begin." To get started:

- Create your first project with **Project ▸ New Project…** (see the [Quick Start](03-quick-start.md)), or
- Open an existing `.rx14proj`, or import an [OLS project](07-ols-import.md).

Set your preferences early if you like — choose a colour theme and interface language in **Settings** and the **Miscellaneous ▸ Language** submenu ([Settings & Localization](15-settings.md)). Auto-save is on by default (5 seconds after your last edit).

## Account and licensing

romHEX 14 comes in two editions:

- **Community** — the free build. All core calibration features are available: projects, A2L/OLS/KP import, map editing, Tuning Branches, the Differences panel, map packs and patches.
- **Pro** — adds an account-based set of premium features: **AI tuning functions**, **AI map translation**, **checksum verify/correct**, **compare ROM/version** and **compare hex**, **auto-detect ECU**, the **DTC manager**, and update checking.

In a Pro build, sign in from **Help ▸ Account / Sign in…**. Your account unlocks specific modules (for example AI tuning tools and AI map translation); features whose module is not active show a note explaining what is required. Pro features can be purchased from **romhex14.com**. When a premium action is unavailable in your edition, romHEX 14 tells you rather than failing silently.
