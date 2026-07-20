/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>
#include <QCoreApplication>
#include "romdata.h"

namespace ols {

// Writes a .kp map pack in the OLS 5.x "schema 750" layout: the OLS file
// wrapper around a ZIP whose deflate-compressed "intern" entry holds one
// length-prefixed record per map (address, dimensions, cell size, linear
// scaling and X/Y axis blocks). Produced files round-trip cleanly through
// KpImporter; byte-for-byte compatibility with the original toolchain is not
// asserted (validated against KpImporter, not the vendor tool).
class KpExporter {
    Q_DECLARE_TR_FUNCTIONS(ols::KpExporter)
public:
    struct Result {
        QByteArray data;      // the complete .kp file bytes
        QString    error;
        int        mapCount = 0;
    };

    // romSize is the ROM byte size the addresses index into (used as the record
    // "base" marker, as the vendor format does).
    static Result exportToBytes(const QVector<MapInfo> &maps, uint32_t romSize);
};

} // namespace ols
