/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>

// Minimal, self-contained AES-128 (FIPS-197) with CBC mode.
//
// romHEX14 links no external crypto library, and Qt exposes none, so the VAG
// FRF/ODX extractor needs its own AES for the (optional) per-block encryption
// stage. Correctness is pinned by a FIPS-197 known-answer test in tests/.
//
// Keys are NEVER shipped with the app — the caller supplies the 16-byte key
// and IV (typically loaded from a user-provided key file).
namespace vag {

// AES-128-CBC decrypt. `key` and `iv` must be 16 bytes; `cipher` must be a
// non-zero multiple of 16. Returns the raw plaintext (PKCS#7 padding, if any,
// is left intact — flash blocks are not PKCS#7 padded). Returns empty on a
// malformed argument.
QByteArray aes128CbcDecrypt(const QByteArray &cipher,
                            const QByteArray &key,
                            const QByteArray &iv);

// AES-128-CBC encrypt (same constraints). Provided mainly so the KAT can round
// -trip; also usable to re-pack a block.
QByteArray aes128CbcEncrypt(const QByteArray &plain,
                            const QByteArray &key,
                            const QByteArray &iv);

// Single-block ECB primitives (16-byte in/out). Exposed for the known-answer
// test; block ciphers are rarely used directly in ECB for real data.
QByteArray aes128EcbEncryptBlock(const QByteArray &block16, const QByteArray &key);
QByteArray aes128EcbDecryptBlock(const QByteArray &block16, const QByteArray &key);

} // namespace vag
