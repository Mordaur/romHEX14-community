/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "TuneReport.h"

#include <QLocale>
#include <cmath>
#include <limits>

namespace tunereport {

namespace {

double applyScale(const MapInfo &m, double raw)
{
    if (!m.hasScaling) return raw;
    const CompuMethod &cm = m.scaling;
    switch (cm.type) {
    case CompuMethod::Type::Linear:
        return cm.linA * raw + cm.linB;
    case CompuMethod::Type::RationalFunction: {
        // Stored coeffs describe phys->raw; invert the common linear case
        // (a==d==0) so the report shows physical units.
        const double a = cm.rfA, b = cm.rfB, c = cm.rfC,
                     d = cm.rfD, e = cm.rfE, f = cm.rfF;
        if (a == 0 && d == 0 && b != 0)
            return (raw * f - c) / (b - raw * e);
        return raw;
    }
    default:
        return raw;
    }
}

long readCell(const QByteArray &rom, int pos, int sz, bool bigEndian, bool sgn)
{
    if (pos < 0 || pos + sz > rom.size() || sz <= 0) return 0;
    const auto *p = reinterpret_cast<const unsigned char *>(rom.constData()) + pos;
    unsigned long v = 0;
    if (bigEndian) for (int i = 0; i < sz; i++)      v = (v << 8) | p[i];
    else           for (int i = sz - 1; i >= 0; i--) v = (v << 8) | p[i];
    if (sgn) {
        const unsigned long signBit = 1UL << (sz * 8 - 1);
        if (v & signBit) return static_cast<long>(v) - static_cast<long>(signBit << 1);
    }
    return static_cast<long>(v);
}

QString fmt(double v)
{
    // Compact but readable: integers stay integer, else up to 3 decimals.
    if (std::abs(v - std::llround(v)) < 1e-9)
        return QString::number(static_cast<long long>(std::llround(v)));
    return QLocale::c().toString(v, 'f', 3);
}

QString esc(const QString &s)
{
    QString o = s;
    o.replace('&', "&amp;").replace('<', "&lt;").replace('>', "&gt;");
    return o;
}

} // namespace

QVector<MapChange> analyze(const QVector<MapInfo> &maps,
                           const QByteArray &stock,
                           const QByteArray &tuned)
{
    QVector<MapChange> out;
    out.reserve(maps.size());

    for (const auto &m : maps) {
        const int ds = m.dataSize > 0 ? m.dataSize : 1;
        const int cols = qMax(1, m.dimensions.x);
        const int rows = qMax(1, m.dimensions.y);
        const int cells = cols * rows;
        const int base = static_cast<int>(m.address) + static_cast<int>(m.mapDataOffset);

        MapChange c;
        c.name       = m.name;
        c.folderPath = m.folderPath;
        c.type       = m.type;
        c.unit       = m.hasScaling ? m.scaling.unit : QString();
        c.address    = m.address;
        c.cols       = cols;
        c.rows       = rows;
        c.totalCells = cells;

        double sMin =  std::numeric_limits<double>::infinity();
        double sMax = -std::numeric_limits<double>::infinity();
        double tMin =  std::numeric_limits<double>::infinity();
        double tMax = -std::numeric_limits<double>::infinity();
        double sSum = 0, tSum = 0;
        int counted = 0, changed = 0;

        for (int i = 0; i < cells; i++) {
            const int pos = base + i * ds;
            if (pos + ds > stock.size() || pos + ds > tuned.size()) break;
            const long rs = readCell(stock, pos, ds, m.cellBigEndian, m.dataSigned);
            const long rt = readCell(tuned, pos, ds, m.cellBigEndian, m.dataSigned);
            const double ps = applyScale(m, static_cast<double>(rs));
            const double pt = applyScale(m, static_cast<double>(rt));
            sMin = qMin(sMin, ps); sMax = qMax(sMax, ps); sSum += ps;
            tMin = qMin(tMin, pt); tMax = qMax(tMax, pt); tSum += pt;
            if (rs != rt) changed++;
            counted++;
        }

        if (counted == 0) continue;
        c.changedCells = changed;
        c.changed      = changed > 0;
        c.stockMin = sMin; c.stockMax = sMax; c.stockMean = sSum / counted;
        c.tunedMin = tMin; c.tunedMax = tMax; c.tunedMean = tSum / counted;
        out.append(c);
    }
    return out;
}

QString renderHtml(const QVector<MapChange> &changes, const Options &opt)
{
    int changedMaps = 0, changedCells = 0;
    for (const auto &c : changes)
        if (c.changed) { changedMaps++; changedCells += c.changedCells; }

    QString rows;
    for (const auto &c : changes) {
        if (!c.changed && !opt.includeUnchanged) continue;
        const QString unit = c.unit.isEmpty() ? QString() : QStringLiteral(" ") + esc(c.unit);
        const QString dims = c.rows > 1 ? QStringLiteral("%1×%2").arg(c.cols).arg(c.rows)
                                        : QString::number(c.cols);
        const double meanDelta = c.tunedMean - c.stockMean;
        const QString deltaCls = c.changed
            ? (meanDelta > 0 ? QStringLiteral("up") : meanDelta < 0 ? QStringLiteral("dn")
                                                                    : QStringLiteral(""))
            : QStringLiteral("");
        rows += QStringLiteral(
            "<tr class=\"%1\">"
            "<td class=\"name\">%2%3</td>"
            "<td>%4</td><td class=\"mono\">0x%5</td><td class=\"num\">%6</td>"
            "<td class=\"num\">%7 / %8</td>"
            "<td class=\"num\">%9 / %10</td>"
            "<td class=\"num %11\">%12</td>"
            "<td class=\"num\">%13/%14</td></tr>\n")
            .arg(c.changed ? QStringLiteral("chg") : QStringLiteral("same"))
            .arg(c.folderPath.isEmpty() ? QString()
                 : QStringLiteral("<span class=\"folder\">%1 / </span>").arg(esc(c.folderPath)))
            .arg(esc(c.name))
            .arg(esc(c.type)).arg(c.address, 0, 16).arg(dims)
            .arg(fmt(c.stockMin) + unit).arg(fmt(c.stockMax) + unit)
            .arg(fmt(c.tunedMin) + unit).arg(fmt(c.tunedMax) + unit)
            .arg(deltaCls)
            .arg((meanDelta >= 0 ? QStringLiteral("+") : QString()) + fmt(meanDelta) + unit)
            .arg(c.changedCells).arg(c.totalCells);
    }

    auto metaRow = [](const QString &k, const QString &v) {
        return v.isEmpty() ? QString()
             : QStringLiteral("<div><dt>%1</dt><dd>%2</dd></div>").arg(k, esc(v));
    };

    QString head;
    head += metaRow(QStringLiteral("Project"),  opt.projectName);
    head += metaRow(QStringLiteral("Vehicle"),  opt.vehicle);
    head += metaRow(QStringLiteral("ECU"),      opt.ecuName);
    head += metaRow(QStringLiteral("Software"), opt.softwareVersion);
    head += metaRow(QStringLiteral("Author"),   opt.author);
    head += metaRow(QStringLiteral("Generated"),opt.generatedAt);

    const QString title = opt.projectName.isEmpty()
        ? QStringLiteral("Tuning Report") : opt.projectName + QStringLiteral(" — Tuning Report");

    return QStringLiteral(R"HTML(<!doctype html>
<html><head><meta charset="utf-8"><title>%1</title>
<style>
 body{font:13px/1.5 -apple-system,Segoe UI,Roboto,sans-serif;color:#1a1a1a;margin:32px;}
 h1{font-size:20px;margin:0 0 4px;} .sub{color:#666;margin:0 0 18px;}
 dl.meta{display:flex;flex-wrap:wrap;gap:4px 28px;margin:0 0 18px;padding:12px 16px;
   background:#f6f7f9;border:1px solid #e3e6ea;border-radius:8px;}
 dl.meta div{display:flex;gap:8px;} dl.meta dt{font-weight:600;color:#555;margin:0;}
 dl.meta dd{margin:0;}
 .summary{display:flex;gap:24px;margin:0 0 18px;}
 .summary .card{flex:1;padding:12px 16px;border:1px solid #e3e6ea;border-radius:8px;text-align:center;}
 .summary .n{font-size:24px;font-weight:700;} .summary .l{color:#666;font-size:12px;}
 table{border-collapse:collapse;width:100%;font-size:12px;}
 th,td{border-bottom:1px solid #ececec;padding:5px 8px;text-align:left;vertical-align:top;}
 th{background:#fafbfc;font-weight:600;border-bottom:2px solid #e3e6ea;position:sticky;top:0;}
 td.num{text-align:right;white-space:nowrap;} td.mono{font-family:ui-monospace,Consolas,monospace;}
 td.name{font-weight:600;} .folder{color:#888;font-weight:400;}
 tr.chg{background:#fffdf5;} .up{color:#0a7d29;} .dn{color:#b32020;}
 .note{margin:18px 0;padding:10px 14px;background:#eef6ff;border:1px solid #cfe3fb;border-radius:8px;}
 footer{margin-top:24px;color:#999;font-size:11px;}
 @media print{body{margin:0;} th{position:static;}}
</style></head><body>
<h1>%1</h1>
<p class="sub">%2 changed of %3 maps compared &middot; %4 cells modified</p>
<dl class="meta">%5</dl>
<div class="summary">
 <div class="card"><div class="n">%2</div><div class="l">maps changed</div></div>
 <div class="card"><div class="n">%4</div><div class="l">cells modified</div></div>
 <div class="card"><div class="n">%3</div><div class="l">maps compared</div></div>
</div>
%6
<table><thead><tr>
 <th>Map</th><th>Type</th><th>Address</th><th>Dims</th>
 <th>%7 (min/max)</th><th>%8 (min/max)</th><th>Δ mean</th><th>Cells</th>
</tr></thead><tbody>
%9
</tbody></table>
<footer>Generated by romHEX14.</footer>
</body></html>
)HTML")
        .arg(esc(title))
        .arg(changedMaps).arg(changes.size()).arg(changedCells)
        .arg(head)
        .arg(opt.checksumNote.isEmpty() ? QString()
             : QStringLiteral("<div class=\"note\">%1</div>").arg(esc(opt.checksumNote)))
        .arg(esc(opt.stockLabel)).arg(esc(opt.tunedLabel))
        .arg(rows);
}

QString generateHtml(const QVector<MapInfo> &maps,
                     const QByteArray &stock,
                     const QByteArray &tuned,
                     const Options &opt)
{
    return renderHtml(analyze(maps, stock, tuned), opt);
}

} // namespace tunereport
