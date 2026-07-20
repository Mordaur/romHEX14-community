/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>
#include "romdata.h"

// TunerPro XDF (.xdf) definition import/export.
//
// XDF is a plain-XML calibration-definition format (no ROM bytes). romHEX14
// maps XDFCONSTANT -> VALUE maps and XDFTABLE (x/y/z axes) -> CURVE/MAP maps,
// with CATEGORY membership becoming the folder path and MATH equations becoming
// linear scaling. Addresses are file offsets relative to XDFHEADER/BASEOFFSET.
namespace xdf {

struct ImportResult {
    QVector<MapInfo> maps;
    uint32_t         baseOffset = 0;   // XDFHEADER BASEOFFSET (added to mmedaddress)
    uint32_t         romSize    = 0;   // from REGION size, if present
    QString          error;
    QStringList      warnings;
};

// Parse an XDF document. On malformed XML, `error` is set and `maps` is empty.
ImportResult importFromXml(const QByteArray &xml);

struct ExportOptions {
    uint32_t romSize   = 0;            // REGION size (0 -> omit / default 0x400000)
    uint32_t baseOffset = 0;
    bool     bigEndian = true;         // cell/axis byte order (DEFAULTS lsbfirst)
    QString  description;
};

// Serialize maps to an XDF 1.60 document. Folder paths become CATEGORY entries.
QByteArray exportToXml(const QVector<MapInfo> &maps, const ExportOptions &opt);

// Parse a TunerPro MATH equation (e.g. "X*0.078125", "X*0.75-48", "(X+40)*2",
// "X/4") into a linear CompuMethod. Returns false if the equation is not a
// recognized linear form (caller then leaves the value unscaled).
bool parseLinearEquation(const QString &equation, double *a, double *b);

// Build the "X*a", "X*a+b", "X" equation string for a linear scaling.
QString buildLinearEquation(double a, double b);

} // namespace xdf
