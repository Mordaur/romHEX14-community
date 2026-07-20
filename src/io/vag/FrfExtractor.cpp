/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "FrfExtractor.h"
#include "FrfCodec.h"
#include "Aes128.h"
#include "../ols/ZipDecompressor.h"

namespace vag {

int FrfResult::primaryBlockIndex() const
{
    int best = -1, bestSize = -1;
    for (int i = 0; i < blocks.size(); ++i) {
        if (blocks[i].decoded && blocks[i].data.size() > bestSize) {
            bestSize = blocks[i].data.size();
            best = i;
        }
    }
    return best;
}

namespace {

quint16 rd16(const QByteArray &b, int off) {
    if (off < 0 || off + 2 > b.size()) return 0;
    const auto *p = reinterpret_cast<const uint8_t*>(b.constData()) + off;
    return quint16(p[0] | (p[1] << 8));
}
quint32 rd32(const QByteArray &b, int off) {
    if (off < 0 || off + 4 > b.size()) return 0;
    const auto *p = reinterpret_cast<const uint8_t*>(b.constData()) + off;
    return quint32(p[0] | (p[1] << 8) | (p[2] << 16) | (quint32(p[3]) << 24));
}

// Pull the most relevant entry out of a ZIP archive (central-directory based).
// Prefers a name containing ".odx"/".frf"/".xml"; otherwise the largest entry.
// Returns the inflated bytes, or empty with *err set.
QByteArray unzipOdx(const QByteArray &zip, QString *err)
{
    // Locate End Of Central Directory (scan back for PK\x05\x06).
    const int n = zip.size();
    int eocd = -1;
    for (int i = n - 22; i >= 0 && i >= n - 22 - 65536; --i) {
        if (rd32(zip, i) == 0x06054b50u) { eocd = i; break; }
    }
    if (eocd < 0) { if (err) *err = QStringLiteral("not a ZIP (no EOCD record)"); return {}; }

    const quint16 count   = rd16(zip, eocd + 10);
    const quint32 cdOff   = rd32(zip, eocd + 16);
    if (cdOff >= quint32(n)) { if (err) *err = QStringLiteral("bad central directory offset"); return {}; }

    int    bestLocalOff = -1;
    quint16 bestMethod  = 0;
    quint32 bestComp    = 0, bestUncomp = 0;
    int     bestScore   = -1;    // name match (2) > size fallback
    quint32 bestSize    = 0;

    int p = int(cdOff);
    for (int i = 0; i < count; ++i) {
        if (rd32(zip, p) != 0x02014b50u) break;   // central file header sig
        const quint16 method  = rd16(zip, p + 10);
        const quint32 comp    = rd32(zip, p + 20);
        const quint32 uncomp  = rd32(zip, p + 24);
        const quint16 nameLen = rd16(zip, p + 28);
        const quint16 extraLen= rd16(zip, p + 30);
        const quint16 cmtLen  = rd16(zip, p + 32);
        const quint32 localOff= rd32(zip, p + 42);
        const QString name    = QString::fromUtf8(zip.mid(p + 46, nameLen)).toLower();

        int score = 0;
        if (name.contains(".odx") || name.contains(".frf") || name.endsWith(".xml"))
            score = 2;
        // Prefer name matches; among equals, the larger uncompressed entry.
        if (score > bestScore || (score == bestScore && uncomp > bestSize)) {
            bestScore = score; bestSize = uncomp;
            bestLocalOff = int(localOff); bestMethod = method;
            bestComp = comp; bestUncomp = uncomp;
        }
        p += 46 + nameLen + extraLen + cmtLen;
    }

    if (bestLocalOff < 0) { if (err) *err = QStringLiteral("ZIP has no usable entry"); return {}; }

    // Re-read the local header to find the true data offset.
    if (rd32(zip, bestLocalOff) != 0x04034b50u) {
        if (err) *err = QStringLiteral("bad local file header"); return {};
    }
    const quint16 lnameLen  = rd16(zip, bestLocalOff + 26);
    const quint16 lextraLen = rd16(zip, bestLocalOff + 28);
    const int dataOff = bestLocalOff + 30 + lnameLen + lextraLen;
    if (dataOff < 0 || dataOff + int(bestComp) > n) {
        if (err) *err = QStringLiteral("ZIP entry data out of range"); return {};
    }
    const QByteArray comp = zip.mid(dataOff, int(bestComp));

    if (bestMethod == 0) return comp;              // stored
    if (bestMethod == 8)                            // deflate
        return ols::ZipDecompressor::decompress(comp, qsizetype(bestUncomp), err);

    if (err) *err = QStringLiteral("unsupported ZIP method %1").arg(bestMethod);
    return {};
}

} // namespace

FrfResult FrfExtractor::extract(const QByteArray &containerBytes, const Keys &keys)
{
    FrfResult res;

    if (containerBytes.size() < 4) {
        res.error = QStringLiteral("file too small");
        return res;
    }

    // ── Stage 1: obtain the ODX XML ────────────────────────────────────────
    res.stage = QStringLiteral("detect");
    QByteArray odxXml;
    const bool looksZip = (rd32(containerBytes, 0) == 0x04034b50u);
    const char c0 = containerBytes.isEmpty() ? 0 : containerBytes.at(0);
    const bool looksXml = (c0 == '<');

    if (looksXml) {
        odxXml = containerBytes;                    // already a bare ODX
        res.stage = QStringLiteral("odx");
    } else {
        QByteArray zip = containerBytes;
        if (!looksZip) {
            // Raw .frf → XOR to reveal the ZIP.
            if (keys.xorKey.isEmpty()) {
                res.error = QStringLiteral(
                    "This looks like a raw .frf — supply the VW frf.key to decode it.");
                return res;
            }
            res.stage = QStringLiteral("xor");
            zip = frfXor(containerBytes, keys.xorKey);
            if (rd32(zip, 0) != 0x04034b50u) {
                res.error = QStringLiteral(
                    "XOR did not yield a ZIP — the frf.key is likely wrong.");
                return res;
            }
        }
        res.stage = QStringLiteral("unzip");
        QString zerr;
        odxXml = unzipOdx(zip, &zerr);
        if (odxXml.isEmpty()) {
            res.error = QStringLiteral("ZIP extract failed: %1").arg(zerr);
            return res;
        }
    }

    // ── Stage 2: parse ODX blocks ──────────────────────────────────────────
    res.stage = QStringLiteral("odx-parse");
    QString perr;
    const QVector<OdxBlock> odx = parseOdxBlocks(odxXml, &perr);
    if (odx.isEmpty()) {
        res.error = perr.isEmpty() ? QStringLiteral("no flash blocks found in ODX") : perr;
        return res;
    }

    // ── Stage 3: decode each block (AES → LZSS) ────────────────────────────
    res.stage = QStringLiteral("decode");
    const bool haveAes = keys.aesKey.size() == 16 && keys.aesIv.size() == 16;

    for (const OdxBlock &ob : odx) {
        FrfBlock fb;
        fb.id        = ob.id;
        fb.address   = ob.address;
        fb.rawSize   = ob.payload.size();
        fb.encrypted = !(ob.encryption == QChar('0') || ob.encryption.isNull());
        fb.compressed = (ob.compression == QChar('A') || ob.compression == QChar('a'));

        QByteArray payload = ob.payload;

        if (fb.encrypted) {
            res.anyEncrypted = true;
            if (!haveAes) {
                fb.needsKey = true;
                fb.note = QStringLiteral("AES-encrypted — supply key/IV to decode");
                res.anyNeedsKey = true;
                res.blocks.append(fb);
                continue;
            }
            if (payload.size() % 16 != 0) {
                fb.note = QStringLiteral("encrypted payload not a multiple of 16 bytes");
                res.blocks.append(fb);
                continue;
            }
            payload = aes128CbcDecrypt(payload, keys.aesKey, keys.aesIv);
            if (payload.isEmpty()) {
                fb.needsKey = true;
                fb.note = QStringLiteral("AES decrypt failed — wrong key/IV?");
                res.anyNeedsKey = true;
                res.blocks.append(fb);
                continue;
            }
        }

        if (fb.compressed) {
            const QByteArray dec = lzssDecompress(payload);
            if (dec.isEmpty() && !payload.isEmpty()) {
                fb.note = QStringLiteral("LZSS decompress produced no output");
                res.blocks.append(fb);
                continue;
            }
            payload = dec;
        }

        fb.data    = payload;
        fb.decoded = true;
        res.blocks.append(fb);
    }

    res.ok = true;
    res.stage = QStringLiteral("done");
    return res;
}

} // namespace vag
