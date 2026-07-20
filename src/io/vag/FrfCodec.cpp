/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// FRF/ODX codec primitives. Algorithms follow bri3d/VW_Flash
// (frf/decryptfrf.py, lib/lzss/lzss.c, extractodx.py).

#include "FrfCodec.h"

#include <QXmlStreamReader>

namespace vag {

QByteArray frfXor(const QByteArray &data, const QByteArray &key)
{
    if (key.isEmpty()) return {};
    QByteArray out;
    out.resize(data.size());
    const auto *k = reinterpret_cast<const uint8_t *>(key.constData());
    const int klen = key.size();

    int keyIndex = 0;
    uint8_t firstSeed = 0;
    uint8_t secondSeed = 1;
    for (int i = 0; i < data.size(); ++i) {
        const uint8_t keyByte = k[keyIndex];
        firstSeed = uint8_t((firstSeed + keyByte) * 3);
        const uint8_t mask = uint8_t(firstSeed ^ 0xFF ^ secondSeed ^ keyByte);
        out[i] = char(uint8_t(data.at(i)) ^ mask);
        secondSeed = uint8_t((secondSeed + 1) * firstSeed);
        keyIndex = (keyIndex + 1) % klen;
    }
    return out;
}

// ── Audi LZSS ────────────────────────────────────────────────────────────────

static constexpr int kWindow = 1024;

QByteArray lzssDecompress(const QByteArray &in)
{
    QByteArray out;
    QByteArray window(kWindow, ' ');
    int nextChar = 0;
    int pos = 0;
    const auto get = [&](int &c) -> bool {
        if (pos >= in.size()) return false;
        c = uint8_t(in.at(pos++));
        return true;
    };

    uint8_t flags = 0;
    int flagsUsed = 7;
    while (true) {
        flags <<= 1;
        flagsUsed++;
        if (flagsUsed == 8) {
            int c;
            if (!get(c)) break;
            flags = uint8_t(c);
            flagsUsed = 0;
        }
        if ((flags & 0x80) == 0) {                     // literal
            int c;
            if (!get(c)) break;
            out.append(char(c));
            window[nextChar] = char(c);
            nextChar = (nextChar + 1) % kWindow;
        } else {                                       // back-reference
            int b0, b1;
            if (!get(b0)) break;
            if (!get(b1)) break;
            int offset = b1 + ((b0 & 0x03) << 8);
            offset = kWindow - offset;
            const int length = b0 >> 2;
            QByteArray look(length, '\0');
            for (int i = 0; i < length; ++i) {
                const char c = window[(nextChar + offset + i) % kWindow];
                out.append(c);
                look[i] = c;
            }
            for (int i = 0; i < length; ++i)
                window[(nextChar + i) % kWindow] = look[i];
            nextChar = (nextChar + length) % kWindow;
        }
    }
    return out;
}

// Greedy compressor producing streams the decompressor above reads back exactly.
// Mirrors the same window model (initial fill with spaces, 6-bit length /
// 10-bit offset), so encode->decode is a faithful round-trip.
QByteArray lzssCompress(const QByteArray &in)
{
    QByteArray out;
    QByteArray window(kWindow, ' ');
    int nextChar = 0;

    uint8_t flags = 0;
    int flagCount = 0;
    int flagPos = -1;                 // index in `out` of the pending flag byte

    auto beginFlagGroup = [&]() {
        flagPos = out.size();
        out.append('\0');
        flags = 0;
        flagCount = 0;
    };
    auto pushBit = [&](bool set) {
        if (flagCount == 0) beginFlagGroup();
        flags = uint8_t((flags << 1) | (set ? 1 : 0));
        flagCount++;
        if (flagCount == 8) {
            out[flagPos] = char(flags);
            flagCount = 0;
        }
    };

    int i = 0;
    const int n = in.size();
    while (i < n) {
        // Find the longest match in the window (max length 63 = 6 bits).
        int bestLen = 0, bestOff = 0;
        const int maxLen = qMin(63, n - i);
        if (maxLen >= 2) {
            for (int off = 1; off <= kWindow; ++off) {
                // Cap the match at `off` bytes: this decoder reads the entire
                // match from the pre-update window, so overlapping (run-length)
                // matches would decode wrong. Non-overlap keeps it faithful.
                const int lim = qMin(maxLen, off);
                int len = 0;
                while (len < lim) {
                    const char wc = window[(nextChar - off + len + kWindow * 2) % kWindow];
                    if (wc != in.at(i + len)) break;
                    ++len;
                }
                if (len > bestLen) { bestLen = len; bestOff = off; }
            }
        }
        if (bestLen >= 2) {
            pushBit(true);
            // Encode offset as WINDOW - dispFromStart where the decoder computes
            // window[(nextChar + (WINDOW - stored) + k)]. Match starts at
            // nextChar-bestOff, so stored offset value must satisfy
            // (WINDOW - stored) ≡ -bestOff (mod WINDOW) => stored = bestOff.
            const int stored = bestOff;
            const uint8_t b0 = uint8_t((bestLen << 2) | ((stored >> 8) & 0x03));
            const uint8_t b1 = uint8_t(stored & 0xFF);
            out.append(char(b0));
            out.append(char(b1));
            for (int k = 0; k < bestLen; ++k) {
                window[nextChar] = in.at(i + k);
                nextChar = (nextChar + 1) % kWindow;
            }
            i += bestLen;
        } else {
            pushBit(false);
            out.append(in.at(i));
            window[nextChar] = in.at(i);
            nextChar = (nextChar + 1) % kWindow;
            ++i;
        }
    }
    if (flagCount != 0)               // flush partial flag group (left-aligned)
        out[flagPos] = char(uint8_t(flags << (8 - flagCount)));
    return out;
}

// ── ODX flash-block parsing ──────────────────────────────────────────────────

static QByteArray decodePayload(const QString &text, const QString &fmt)
{
    const QString t = text.trimmed();
    if (fmt.contains(QLatin1String("HEX"), Qt::CaseInsensitive)
        || fmt.isEmpty()) {
        // Default and most common: hex string.
        QString hex = t;
        hex.remove(QChar(' ')).remove(QChar('\n')).remove(QChar('\r')).remove(QChar('\t'));
        return QByteArray::fromHex(hex.toLatin1());
    }
    return QByteArray::fromBase64(t.toLatin1());
}

QVector<OdxBlock> parseOdxBlocks(const QByteArray &odxXml, QString *error)
{
    QVector<OdxBlock> blocks;
    QXmlStreamReader r(odxXml);

    while (!r.atEnd()) {
        if (r.readNext() != QXmlStreamReader::StartElement) continue;
        if (r.name() != QLatin1String("FLASHDATA")) continue;

        OdxBlock b;
        b.id = r.attributes().value(QStringLiteral("ID")).toString();
        QString method, dataFormat, dataText, address;

        int depth = 1;
        while (depth > 0 && !r.atEnd()) {
            const auto tok = r.readNext();
            if (tok == QXmlStreamReader::StartElement) {
                depth++;
                const QStringView n = r.name();
                if (n == QLatin1String("DATAFORMAT"))
                    dataFormat = r.attributes().value(QStringLiteral("SELECTION")).toString();
                else if (n == QLatin1String("DATA"))
                    dataText = r.readElementText(), depth--;   // readElementText consumes end
                else if (n == QLatin1String("ADDRESS") || n == QLatin1String("START-ADDRESS"))
                    address = r.readElementText(), depth--;
                else if (n == QLatin1String("ENCRYPT-COMPRESS-METHOD")
                         || n == QLatin1String("METHOD"))
                    method = r.readElementText(), depth--;
            } else if (tok == QXmlStreamReader::EndElement) {
                depth--;
            }
        }

        if (method.size() >= 2) {
            b.compression = method.at(0);
            b.encryption  = method.at(1);
        } else {
            b.compression = QChar('0');
            b.encryption  = QChar('0');
        }
        if (!address.isEmpty())
            b.address = address.startsWith(QLatin1String("0x"))
                ? address.mid(2).toUInt(nullptr, 16) : address.toUInt(nullptr, 0);
        b.payload = decodePayload(dataText, dataFormat);
        blocks.append(b);
    }

    if (r.hasError()) {
        if (error) *error = QStringLiteral("ODX parse error: %1").arg(r.errorString());
        return {};
    }
    return blocks;
}

} // namespace vag
