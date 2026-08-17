/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "uiscale.h"
#include "appconstants.h"

#include <QApplication>
#include <QCoreApplication>
#include <QProcess>
#include <QScreen>
#include <QSettings>
#include <QWidget>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

#ifdef Q_OS_MACOS
#include <CoreGraphics/CoreGraphics.h>
#endif

namespace {

// Reference density: ~110 DIP/inch is the classic "100%" desktop (27" QHD,
// Retina default modes land near it too). Screens noticeably denser than
// that get scaled up.
constexpr double kReferenceDip = 110.0;

// Large desk monitors (>= 32" diagonal) sit farther from the eye than a
// laptop panel, so the same physical text size reads smaller — nudge up.
constexpr double kLargeMonitorDiag   = 32.0;
constexpr double kLargeMonitorBoost  = 1.15;

// Below this ratio the OS default already looks right (covers Retina
// laptops at ~125 DIP/inch); avoids pointless 1.05-1.15 scaling.
constexpr double kDeadZone = 1.2;

constexpr double kMinScale = 1.0;
constexpr double kMaxScale = 3.0;

// Minimum logical (DIP) canvas the app needs to stay operable: the Settings
// dialog (660x560) plus window chrome and some margin. Any scale that would
// shrink the screen below this leaves buttons unreachable — never allow it.
constexpr double kMinUsableW = 720.0;
constexpr double kMinUsableH = 640.0;

// Largest scale at which a screen of wDip x hDip points keeps the minimum
// usable canvas on screen.
double fitCap(double wDip, double hDip)
{
    if (wDip <= 0.0 || hDip <= 0.0)
        return kMaxScale;
    return std::max(kMinScale,
                    std::min({wDip / kMinUsableW, hDip / kMinUsableH,
                              kMaxScale}));
}

// Read a key from the canonical store, falling back to the legacy CT14/RX14
// store (pre-migration installs kept ui/scale there).
QVariant settingWithLegacyFallback(const char *key, const QVariant &def)
{
    QSettings canonical(QString::fromUtf8(rx14::kOrgName),
                        QString::fromUtf8(rx14::kAppName));
    if (canonical.contains(QString::fromUtf8(key)))
        return canonical.value(QString::fromUtf8(key));
    QSettings legacy(QString::fromUtf8(rx14::kOrgName),
                     QString::fromUtf8(rx14::kLegacyAppName));
    return legacy.value(QString::fromUtf8(key), def);
}

} // namespace

namespace UiScale {

double recommendedFromMetrics(double dipWidth, double physWidthMm,
                              double diagInches)
{
    // Bogus EDID data (0 or a few mm) — trust the OS default.
    if (dipWidth <= 0.0 || physWidthMm <= 20.0)
        return 1.0;

    const double dipPerInch = dipWidth / (physWidthMm / 25.4);
    const double distanceFactor =
        (diagInches >= kLargeMonitorDiag) ? kLargeMonitorBoost : 1.0;
    const double raw = (dipPerInch / kReferenceDip) * distanceFactor;

    if (raw < kDeadZone)
        return 1.0;

    // Quarter steps keep the result predictable (125%, 150%, ...).
    const double snapped = std::round(raw * 4.0) / 4.0;
    return std::clamp(snapped, kMinScale, kMaxScale);
}

double recommendedForScreen(const QScreen *screen)
{
    if (!screen)
        return 1.0;
    const QSizeF mm = screen->physicalSize();
    // QScreen::size() is in device-independent pixels, i.e. already divided
    // by the QT_SCALE_FACTOR we exported — multiply it back out to get real
    // points, or a session running at 1.5x would "recommend" 1.0 forever.
    const double dipW = screen->size().width() * appliedScale();
    const double diagIn =
        std::hypot(mm.width(), mm.height()) / 25.4;
    const double rec = recommendedFromMetrics(dipW, mm.width(), diagIn);
    return std::min(rec, fitCapForScreen(screen));
}

double fitCapForScreen(const QScreen *screen)
{
    if (!screen)
        return kMaxScale;
    const double applied = appliedScale();
    return fitCap(screen->size().width() * applied,
                  screen->size().height() * applied);
}

QString mode()
{
    const QString m = settingWithLegacyFallback(kModeKey,
                                                QStringLiteral("auto"))
                          .toString();
    return (m == QLatin1String("manual")) ? QStringLiteral("manual")
                                          : QStringLiteral("auto");
}

double manualScale()
{
    const double v = settingWithLegacyFallback(kManualKey, 1.0).toDouble();
    return std::clamp(v, kMinScale, kMaxScale);
}

double appliedScale()
{
    bool ok = false;
    const double v = qEnvironmentVariable("QT_SCALE_FACTOR").toDouble(&ok);
    return (ok && v > 0.0) ? v : 1.0;
}

double startupScale(const QRect &lastScreenGeom)
{
    const bool manual = (mode() == QLatin1String("manual"));

#ifdef Q_OS_MACOS
    // Enumerate displays via CoreGraphics — safe before QApplication and
    // doesn't bounce a Dock icon the way a probe QGuiApplication would.
    CGDirectDisplayID ids[16];
    uint32_t count = 0;
    if (CGGetActiveDisplayList(16, ids, &count) != kCGErrorSuccess || count == 0)
        return manual ? manualScale() : 1.0;

    CGDirectDisplayID chosen = CGMainDisplayID();
    if (!lastScreenGeom.isNull()) {
        // Pick the display whose bounds best overlap the screen the window
        // last lived on (CG global points match QScreen::geometry()).
        qint64 bestArea = 0;
        for (uint32_t i = 0; i < count; ++i) {
            const CGRect b = CGDisplayBounds(ids[i]);
            const QRect r(int(b.origin.x), int(b.origin.y),
                          int(b.size.width), int(b.size.height));
            const QRect inter = r.intersected(lastScreenGeom);
            const qint64 area = qint64(inter.width()) * inter.height();
            if (area > bestArea) {
                bestArea = area;
                chosen = ids[i];
            }
        }
    }

    const CGSize mm = CGDisplayScreenSize(chosen);
    double dipWidth = 0.0, dipHeight = 0.0;
    if (CGDisplayModeRef dm = CGDisplayCopyDisplayMode(chosen)) {
        dipWidth  = double(CGDisplayModeGetWidth(dm));   // points, not pixels
        dipHeight = double(CGDisplayModeGetHeight(dm));
        CGDisplayModeRelease(dm);
    }

    // Whatever the source of the factor, never scale past the point where
    // the minimum usable canvas stops fitting this display.
    const double cap = fitCap(dipWidth, dipHeight);
    if (manual)
        return std::min(manualScale(), cap);

    const double diagIn = std::hypot(mm.width, mm.height) / 25.4;
    return std::min(recommendedFromMetrics(dipWidth, mm.width, diagIn), cap);
#else
    // Qt 6 already applies correct per-monitor DPI on Windows/Linux.
    Q_UNUSED(lastScreenGeom);
    return manual ? manualScale() : 1.0;
#endif
}

bool requestRestart()
{
    // Close every window first so geometry / last-screen state is persisted
    // and unsaved-changes prompts get their say.
    QCoreApplication *app = QCoreApplication::instance();
    if (!app)
        return false;
    const auto topLevels = QApplication::topLevelWidgets();
    for (QWidget *w : topLevels) {
        if (w->isVisible() && !w->close())
            return false;               // user cancelled a close prompt
    }

    QProcess proc;
    proc.setProgram(QCoreApplication::applicationFilePath());
    proc.setArguments(QCoreApplication::arguments().mid(1));
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // Tells the child to wait for our single-instance lock, and makes sure it
    // recomputes its own scale instead of inheriting this process's factor.
    env.insert(QStringLiteral("RX14_RESTART"), QStringLiteral("1"));
    env.remove(QStringLiteral("QT_SCALE_FACTOR"));
    proc.setProcessEnvironment(env);
    proc.startDetached();
    QCoreApplication::quit();
    return true;
}

} // namespace UiScale
