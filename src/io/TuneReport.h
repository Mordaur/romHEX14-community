/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>
#include "romdata.h"

// Generates a self-contained, printable HTML tuning report: for every map that
// differs between a stock and a modified ROM image, it lists the physical
// value range before/after and the change. Pure function — no Qt Widgets, no
// I/O — so it is unit-testable and callable from both the app and tools.
namespace tunereport {

struct Options {
    QString projectName;
    QString ecuName;
    QString vehicle;
    QString author;
    QString softwareVersion;   // ROM SW version / part number if known
    QString generatedAt;       // caller-supplied timestamp string (may be empty)
    QString stockLabel = QStringLiteral("Original");
    QString tunedLabel = QStringLiteral("Modified");
    QString checksumNote;      // optional free text (e.g. "Checksums corrected")
    bool    includeUnchanged = false;
};

struct MapChange {
    QString  name;
    QString  folderPath;
    QString  type;
    QString  unit;
    uint32_t address = 0;
    int      cols = 1;
    int      rows = 1;
    int      changedCells = 0;
    int      totalCells = 0;
    double   stockMin = 0, stockMax = 0, stockMean = 0;
    double   tunedMin = 0, tunedMax = 0, tunedMean = 0;
    bool     changed = false;
};

// Compute the per-map change set (also used directly by tests).
QVector<MapChange> analyze(const QVector<MapInfo> &maps,
                           const QByteArray &stock,
                           const QByteArray &tuned);

// Render the change set + metadata as a full HTML document.
QString renderHtml(const QVector<MapChange> &changes, const Options &opt);

// Convenience: analyze + render in one call.
QString generateHtml(const QVector<MapInfo> &maps,
                     const QByteArray &stock,
                     const QByteArray &tuned,
                     const Options &opt);

} // namespace tunereport
