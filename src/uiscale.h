/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include <QRect>
#include <QString>

class QScreen;

// Interface (UI) scale handling.
//
// Qt cannot change QT_SCALE_FACTOR after QApplication exists, so the flow is:
//   * main() calls startupScale() BEFORE creating QApplication and exports the
//     result via QT_SCALE_FACTOR.
//   * "auto" mode derives a scale from the physical pixel density of the
//     screen the window last lived on (queried through CoreGraphics on macOS,
//     where this is needed; other platforms already get correct per-monitor
//     scaling from Qt and stay at 1.0).
//   * "manual" mode uses the user-chosen factor from Settings -> Display.
//   * Changing the effective scale at runtime is done with an in-place
//     relaunch (requestRestart()).
namespace UiScale {

// QSettings keys (canonical store, rx14::appSettings()).
inline constexpr auto kModeKey       = "ui/scaleMode";       // "auto" | "manual"
inline constexpr auto kManualKey     = "ui/scale";           // double, 1.0 .. 3.0
inline constexpr auto kLastScreenKey = "ui/lastScreenGeom";  // QRect of last screen

// Pure recommendation formula (unit-testable, no Qt display objects).
//   dipWidth    - horizontal resolution in device-independent pixels (points)
//   physWidthMm - physical width of the panel in millimetres
//   diagInches  - physical diagonal in inches (distance-factor heuristic)
double recommendedFromMetrics(double dipWidth, double physWidthMm,
                              double diagInches);

// Recommendation for a live QScreen (runtime, app already up).
double recommendedForScreen(const QScreen *screen);

// Largest scale at which the app's minimum usable canvas (Settings dialog +
// chrome) still fits this screen. Manual scale is clamped to this at startup;
// the Settings slider also caps its range with it.
double fitCapForScreen(const QScreen *screen);

// Scale to export at startup. Resolves mode/manual value from settings
// (canonical store with legacy-store fallback) and, in auto mode, probes the
// display whose bounds best match lastScreenGeom (falling back to the main
// display). Safe to call before QApplication exists.
double startupScale(const QRect &lastScreenGeom);

// Mode / manual value with legacy-store fallback.
QString mode();          // "auto" (default) or "manual"
double  manualScale();   // clamped to [1.0, 3.0]

// The factor exported through QT_SCALE_FACTOR at startup (1.0 if none).
double appliedScale();

// Relaunch the app so a new scale takes effect. Closes all windows first
// (letting unsaved-changes prompts run); returns false if the user cancelled.
bool requestRestart();

} // namespace UiScale
