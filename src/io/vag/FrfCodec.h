/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

// Building blocks for VAG .frf / .sgo flash-container extraction.
//
// The full pipeline is:  .frf --XOR--> ZIP --unzip--> ODX XML --> flash blocks
// (per block: AES-128-CBC if encrypted, then Audi LZSS if compressed) --> .bin.
//
// This module implements the parts that are self-contained and unit-testable
// without a proprietary sample: the FRF XOR stream cipher (keyed by the static
// VW frf.key blob), the Audi LZSS decompressor, and the ODX flash-block XML
// parser. The XOR key blob and the per-platform AES key/IV table must be
// supplied to complete a real extraction (see FrfExtractor notes) — algorithm
// and layout follow bri3d/VW_Flash.
namespace vag {

// FRF "recursive XOR" stream cipher. The mask is derived from the key and byte
// position only (independent of the data), so this function is its own inverse:
// applying it twice with the same key returns the original bytes.
QByteArray frfXor(const QByteArray &data, const QByteArray &key);

// Audi LZSS decompression: 1024-byte sliding window, MSB-first flag byte, each
// back-reference is 6-bit length + 10-bit offset. Returns the decompressed bytes.
QByteArray lzssDecompress(const QByteArray &in);

// Matching LZSS compressor (greedy). Primarily for testing lzssDecompress, but
// also usable to re-pack blocks. Not size-optimal.
QByteArray lzssCompress(const QByteArray &in);

// One flash block described by the ODX container.
struct OdxBlock {
    QString  id;
    quint32  address = 0;    // block base address (0 if unknown)
    QChar    compression;    // 'A'/'a' = Audi LZSS, '0'/'1'/none = stored/legacy
    QChar    encryption;     // '0' = none, else AES
    QByteArray payload;      // raw block bytes (still enc/compressed as flagged)
};

// Parse the FLASHDATA blocks out of an ODX XML document. Handles the confirmed
// XPath ./FLASH/ECU-MEMS/ECU-MEM/MEM/FLASHDATAS/FLASHDATA and decodes the
// payload per DATAFORMAT (hex or base64). Errors set `error` and return empty.
QVector<OdxBlock> parseOdxBlocks(const QByteArray &odxXml, QString *error);

} // namespace vag
