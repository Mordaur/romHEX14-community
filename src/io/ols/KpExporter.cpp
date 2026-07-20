/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "KpExporter.h"

#include <QtEndian>
#include <cstring>
#include <zlib.h>

namespace ols {

namespace {

void putU32(QByteArray &b, uint32_t v)
{
    char t[4];
    qToLittleEndian(v, t);
    b.append(t, 4);
}

void putU32At(QByteArray &b, int pos, uint32_t v)
{
    qToLittleEndian(v, b.data() + pos);
}

void putF64(QByteArray &b, double v)
{
    uint64_t bits;
    std::memcpy(&bits, &v, 8);
    char t[8];
    qToLittleEndian(bits, t);
    b.append(t, 8);
}

// One schema-750 record. Layout is chosen so KpImporter::readKpHeader's
// schema-750 gate accepts it (11 zero bytes, kind@+11, cellsize@+23, 0x0A@+27)
// and chooseAddress finds the triplet (which must sit at record offset >= 0x40,
// preceded by the scale/offset doubles) with base == romSize.
QByteArray buildRecord(const MapInfo &m, uint32_t romSize)
{
    QByteArray rec;

    // Name (length-prefixed, NUL-terminated).
    const QByteArray name = m.name.toLatin1();
    putU32(rec, uint32_t(name.size()));
    rec.append(name);
    rec.append('\0');

    const int bodyStart = rec.size();     // == metaOff seen by the importer
    const uint32_t kind = (m.type == "VALUE") ? 2u
                        : (m.type == "CURVE") ? 3u : 4u;
    const int ds   = m.dataSize > 0 ? m.dataSize : 1;
    const int cols = qMax(1, m.dimensions.x);
    const int rows = qMax(1, m.dimensions.y);

    // Fixed header. The "folder reference" pad is 11 bytes; we keep +0..+3 zero
    // (which the importer's schema-750 gate requires) but set +7..+10 to 0xFF so
    // that, read one byte early (at the name's NUL), the importer's *compact*
    // header gate fails its v[2]==0 check — otherwise our record would be
    // misdetected as the compact layout and lose its real dimensions. The
    // importer never interprets these bytes at the correct offset.
    rec.append(7, '\0');                  // +0..+6
    putU32(rec, 0xFFFFFFFFu);             // +7..+10 (folder-ref slot, unused)
    putU32(rec, kind);                    // +11 kind
    putU32(rec, 2);                       // +15 (constant in observed files)
    putU32(rec, 3);                       // +19 (observed 1/3; 3 is the common value)
    putU32(rec, uint32_t(ds));            // +23 cell size in bytes
    putU32(rec, 0x0A);                    // +27 marker

    // Pad so the address triplet lands at record offset 0x40, leaving room for
    // the scale/offset doubles at tripletOff-16 / -8.
    const int tripletOff = 0x40;
    while (rec.size() - bodyStart < tripletOff - 16)
        rec.append('\0');

    // Scale / offset doubles (importer reads these at tripletOff-16 / -8).
    double a = 1.0, b = 0.0;
    if (m.hasScaling && m.scaling.type == CompuMethod::Type::Linear) { a = m.scaling.linA; b = m.scaling.linB; }
    putF64(rec, a);
    putF64(rec, b);

    // Address triplet: [raw][end][base==romSize], then the [cols][rows] pair the
    // importer's schema-750 dimension scan looks for (at triplet+12), then a
    // repeat of raw so chooseAddress' "repeated address" bonus applies.
    const uint32_t raw = m.rawAddress ? m.rawAddress : m.address;
    const uint32_t end = raw + uint32_t(cols * rows * ds);
    const int off = rec.size() - bodyStart;   // record-relative triplet offset
    Q_ASSERT(off >= 0x40);
    Q_UNUSED(off);
    putU32(rec, raw);
    putU32(rec, end);
    putU32(rec, romSize);
    putU32(rec, uint32_t(cols));          // triplet+12
    putU32(rec, uint32_t(rows));          // triplet+16
    putU32(rec, raw);                     // triplet+20 (repeat, within 64 bytes)

    return rec;
}

QByteArray buildIntern(const QVector<MapInfo> &maps, uint32_t romSize)
{
    QByteArray intern;
    // Header: [00][mapCount u32][00][FF FF FF FF][00 x5]; records start at 15.
    intern.append('\0');
    putU32(intern, uint32_t(maps.size()));
    intern.append('\0');
    putU32(intern, 0xFFFFFFFFu);
    intern.append(5, '\0');
    for (const auto &m : maps)
        intern.append(buildRecord(m, romSize));
    return intern;
}

// Raw-deflate (windowBits -15) to match KpImporter's inflate.
QByteArray rawDeflate(const QByteArray &in, bool *ok)
{
    z_stream s;
    std::memset(&s, 0, sizeof(s));
    if (deflateInit2(&s, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8,
                     Z_DEFAULT_STRATEGY) != Z_OK) { *ok = false; return {}; }
    s.next_in  = reinterpret_cast<Bytef *>(const_cast<char *>(in.constData()));
    s.avail_in = uInt(in.size());
    QByteArray out;
    out.resize(int(deflateBound(&s, in.size())));
    s.next_out  = reinterpret_cast<Bytef *>(out.data());
    s.avail_out = uInt(out.size());
    const int r = deflate(&s, Z_FINISH);
    const uInt produced = uInt(out.size()) - s.avail_out;
    deflateEnd(&s);
    if (r != Z_STREAM_END) { *ok = false; return {}; }
    out.truncate(int(produced));
    *ok = true;
    return out;
}

} // namespace

KpExporter::Result KpExporter::exportToBytes(const QVector<MapInfo> &maps, uint32_t romSize)
{
    Result res;
    if (maps.isEmpty()) { res.error = tr("No maps to export"); return res; }
    if (romSize == 0) {
        // Fall back to smallest 64KB-aligned size covering the highest address.
        uint32_t hi = 0;
        for (const auto &m : maps) hi = qMax(hi, m.address + uint32_t(qMax(1, m.length)));
        romSize = ((hi + 0xFFFFu) & ~0xFFFFu);
        if (romSize == 0) romSize = 0x10000;
    }

    const QByteArray intern = buildIntern(maps, romSize);
    bool ok = false;
    const QByteArray comp = rawDeflate(intern, &ok);
    if (!ok) { res.error = tr("Compression failed"); return res; }

    // ── ZIP local file header for the single "intern" entry ──────────────
    const QByteArray fname = QByteArrayLiteral("intern");
    const uint32_t crc = uint32_t(::crc32(0, reinterpret_cast<const Bytef *>(intern.constData()),
                                          uInt(intern.size())));
    QByteArray zip;
    zip.append("PK\x03\x04", 4);
    zip.append(char(0x14)); zip.append('\0');   // version needed 20
    zip.append('\0'); zip.append('\0');         // flags
    zip.append(char(8)); zip.append('\0');       // method = deflate
    zip.append('\0'); zip.append('\0');          // mod time
    zip.append('\0'); zip.append('\0');          // mod date
    putU32(zip, crc);                            // CRC-32
    putU32(zip, uint32_t(comp.size()));          // compressed size
    putU32(zip, uint32_t(intern.size()));        // uncompressed size
    zip.append(char(fname.size())); zip.append('\0');  // filename length
    zip.append('\0'); zip.append('\0');          // extra length
    zip.append(fname);
    zip.append(comp);

    // ── OLS file wrapper (magic string is the on-disk format constant) ────
    QByteArray file;
    putU32(file, 11);                            // magic string length
    file.append("WinOLS File", 11);
    file.append('\0');                           // -> offset 0x10
    putU32(file, 750);                           // format/schema version
    const int sizePos = file.size();
    putU32(file, 0);                             // declared file size (patched below)
    while (file.size() < 0x60) file.append('\0');// header padding before the ZIP
    file.append(zip);
    putU32At(file, sizePos, uint32_t(file.size()));

    res.data = file;
    res.mapCount = maps.size();
    return res;
}

} // namespace ols
