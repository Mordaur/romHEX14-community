/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QString>
#include <QVector>

// Generic, module-agnostic data model for the "module editor" framework.
//
// romHEX14 supports reading and (server-side, paid) editing the data flash of
// vehicle control modules — BCM, CAS, and more. Each module type is described
// by a ModuleProfile that DETECTS its files and DECODES them into this shared
// ModuleInfo, which the read-only info panel renders. Decoding/display is free
// (community + pro); write operations are described here but executed server-
// side behind the Pro paywall.
namespace module {

enum class FieldKind {
    Text,        ///< plain string
    PartNumber,  ///< OEM/supplier part number
    Vin,         ///< vehicle identification number
    Supplier,    ///< module manufacturer
    Status,      ///< an on/off or state value (uses `ok` for coloring)
    CsKey,       ///< 16-byte component-protection / checksum key (hex)
    Hex,         ///< raw hex bytes
};

/// One decoded, displayable field.
struct ModuleField {
    QString   label;
    QString   value;
    FieldKind kind = FieldKind::Text;
    QString   note;          ///< e.g. "@0x21D (+mirror 0x211D)"
    bool      ok   = true;    ///< validation state (e.g. mirror/consistency check)
    bool      statusOn = false;  ///< for Status fields: the on/active state (≠ ok)

    // ── Read/write ─────────────────────────────────────────────────────
    // When editable, the panel renders an input and, on Apply, writes the new
    // bytes to every offset in writeOffsets (primary + mirror copies). For
    // CsKey/Hex the input is hex; length must equal writeLen.
    bool         editable = false;
    QVector<int> writeOffsets;   ///< byte offsets to write (all copies)
    int          writeLen = 0;   ///< expected byte length
};

/// A byte patch produced by an editable field (offset → new bytes).
struct ModulePatch {
    int        offset = 0;
    QByteArray bytes;
};

/// A write operation a module supports (VTS toggle, key programming, …).
/// This only DESCRIBES the operation; execution is server-side and gated
/// behind the Pro entitlement (see FeatureGate + CloudClient).
struct ModuleOperation {
    QString id;          ///< "vts_off"
    QString label;       ///< "Disable Vehicle Tracking (VTS Off)"
    QString group;       ///< "Vehicle Tracking System (PVTS)"
    QString description;
    bool    paywalled  = true;   ///< requires a Pro subscription
    bool    serverSide = true;   ///< executed on the cloud server, never locally
    bool    enabled    = true;   ///< currently applicable to this file's state
};

/// The full decoded view of one module file.
struct ModuleInfo {
    bool    detected = false;
    QString profileId;      ///< "porsche_bcm_front"
    QString moduleType;     ///< "Body Control Module (BCM)"
    QString manufacturer;   ///< "Continental"
    QString variant;        ///< "7PP907064EB"
    QString role;           ///< "Front" / "Rear"

    // Structured vehicle/module metadata for project prefill (brand, VIN, …).
    QString brand;          ///< "Porsche"
    QString vehicleModel;   ///< "Cayenne 958"
    QString vin;            ///< chassis number
    QString ecuLabel;       ///< "BCM Front 7PP907064EB" (for the ECU/variant field)

    QVector<ModuleField>     fields;
    QVector<ModuleOperation> operations;
};

} // namespace module
