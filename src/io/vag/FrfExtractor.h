/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

// End-to-end VAG .frf / .sgo / .odx flash-container extractor.
//
// Pipeline:  .frf --XOR(frf.key)--> ZIP --inflate--> ODX XML
//            --> per FLASHDATA block:  [AES-128-CBC if encrypted]
//                                      [Audi LZSS if compressed]  --> raw block
//
// Built on the verified FrfCodec primitives (frfXor / lzssDecompress /
// parseOdxBlocks), zlib for the ZIP stage, and the in-tree AES-128 for the
// optional encryption stage. No keys are shipped: the caller provides the XOR
// key (VW frf.key) and, for encrypted blocks, the per-platform AES key/IV.
namespace vag {

struct FrfBlock {
    QString    id;
    quint32    address   = 0;
    bool       encrypted = false;   // ODX flagged this block AES-encrypted
    bool       compressed = false;  // ODX flagged Audi LZSS
    bool       decoded   = false;   // produced usable output
    bool       needsKey  = false;   // encrypted but no/invalid AES key supplied
    int        rawSize   = 0;       // payload size inside the ODX
    QByteArray data;                // decoded block bytes (empty if needsKey)
    QString    note;
};

struct FrfResult {
    bool               ok = false;      // container parsed to the block list
    QString            error;           // fatal error (empty if ok)
    QString            stage;           // last pipeline stage reached
    QVector<FrfBlock>  blocks;
    bool               anyEncrypted = false;
    bool               anyNeedsKey  = false;

    // Index of the block most likely to be the tunable calibration image
    // (largest decoded block), or -1 if nothing decoded.
    int primaryBlockIndex() const;
};

class FrfExtractor {
public:
    struct Keys {
        QByteArray xorKey;   // VW frf.key blob (required for real .frf; empty if input is already a ZIP/ODX)
        QByteArray aesKey;   // 16 bytes, optional (for encrypted blocks)
        QByteArray aesIv;    // 16 bytes, optional
    };

    // Extract from raw container bytes. Auto-detects whether the input is a
    // raw .frf (needs XOR), an already-XORed ZIP, or a bare ODX XML.
    static FrfResult extract(const QByteArray &containerBytes, const Keys &keys);
};

} // namespace vag
