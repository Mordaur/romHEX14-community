/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "XdfIo.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QHash>
#include <QMap>
#include <cmath>

namespace xdf {

// ── MATH equation <-> linear scaling ─────────────────────────────────────────

bool parseLinearEquation(const QString &equationIn, double *a, double *b)
{
    QString eq = equationIn;
    eq.remove(QChar(' ')).remove(QChar('\t'));
    if (eq.isEmpty()) return false;

    // Uppercase the variable so "x" and "X" both work.
    eq.replace(QChar('x'), QChar('X'));

    auto num = QStringLiteral("([-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?)");

    // X*A , A*X
    QRegularExpression rxMul1("^X\\*" + num + "$");
    QRegularExpression rxMul2("^" + num + "\\*X$");
    // X*A+B , X*A-B  (B sign captured in the number)
    QRegularExpression rxMulAdd("^X\\*" + num + "([-+]" + "[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?)$");
    // X/A
    QRegularExpression rxDiv("^X/" + num + "$");
    // (X+B)*A  ->  A*X + A*B
    QRegularExpression rxOffMul("^\\(X([-+][0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\\)\\*" + num + "$");
    // X+B , X-B
    QRegularExpression rxAdd("^X([-+][0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?)$");

    QRegularExpressionMatch m;
    if ((m = rxMulAdd.match(eq)).hasMatch()) {
        *a = m.captured(1).toDouble(); *b = m.captured(2).toDouble(); return true;
    }
    if ((m = rxOffMul.match(eq)).hasMatch()) {
        const double off = m.captured(1).toDouble();
        *a = m.captured(2).toDouble(); *b = *a * off; return true;
    }
    if ((m = rxMul1.match(eq)).hasMatch()) { *a = m.captured(1).toDouble(); *b = 0.0; return true; }
    if ((m = rxMul2.match(eq)).hasMatch()) { *a = m.captured(1).toDouble(); *b = 0.0; return true; }
    if ((m = rxDiv.match(eq)).hasMatch()) {
        const double d = m.captured(1).toDouble();
        if (d == 0.0) return false;
        *a = 1.0 / d; *b = 0.0; return true;
    }
    if ((m = rxAdd.match(eq)).hasMatch()) { *a = 1.0; *b = m.captured(1).toDouble(); return true; }
    if (eq == QLatin1String("X"))         { *a = 1.0; *b = 0.0; return true; }
    return false;
}

QString buildLinearEquation(double a, double b)
{
    // Trim trailing zeros for readability without losing precision.
    auto fmt = [](double v) {
        QString s = QString::number(v, 'g', 12);
        return s;
    };
    if (a == 1.0 && b == 0.0) return QStringLiteral("X");
    QString s = QStringLiteral("X*") + fmt(a);
    if (b != 0.0)
        s += (b > 0 ? QStringLiteral("+") : QStringLiteral("-")) + fmt(std::abs(b));
    return s;
}

// ── Import ───────────────────────────────────────────────────────────────────

namespace {

struct Embedded {
    bool     hasAddress = false;
    uint32_t address    = 0;
    int      sizeBits   = 8;
    int      rows       = 1;
    int      cols       = 1;
    uint32_t typeFlags  = 0;   // mmedtypeflags (bit0 often = signed)
};

Embedded readEmbedded(const QXmlStreamAttributes &at)
{
    Embedded e;
    auto num = [&](const QString &k, bool *ok = nullptr) -> long long {
        const QString v = at.value(k).toString();
        if (v.isEmpty()) { if (ok) *ok = false; return 0; }
        if (ok) *ok = true;
        return v.startsWith(QLatin1String("0x")) || v.startsWith(QLatin1String("0X"))
            ? v.mid(2).toLongLong(nullptr, 16) : v.toLongLong(nullptr, 0);
    };
    bool ok = false;
    const long long addr = num(QStringLiteral("mmedaddress"), &ok);
    if (ok) { e.hasAddress = true; e.address = uint32_t(addr); }
    if (at.hasAttribute(QStringLiteral("mmedelementsizebits")))
        e.sizeBits = int(num(QStringLiteral("mmedelementsizebits")));
    if (at.hasAttribute(QStringLiteral("mmedrowcount")))
        e.rows = int(num(QStringLiteral("mmedrowcount")));
    if (at.hasAttribute(QStringLiteral("mmedcolcount")))
        e.cols = int(num(QStringLiteral("mmedcolcount")));
    if (at.hasAttribute(QStringLiteral("mmedtypeflags")))
        e.typeFlags = uint32_t(num(QStringLiteral("mmedtypeflags")));
    return e;
}

int bytesFromBits(int bits) { return bits <= 8 ? 1 : bits <= 16 ? 2 : 4; }

// XDF files exported by ECU tools are frequently Latin-1/Windows-1252 with no
// <?xml encoding> declaration (e.g. a "°C" unit byte), which a UTF-8 parser
// rejects. Decode to a QString ourselves — honor a declared encoding, else use
// UTF-8 when the bytes are valid UTF-8, otherwise fall back to Latin-1.
QString decodeXml(const QByteArray &xml)
{
    // Look for an explicit encoding in the XML declaration (ASCII-safe scan).
    const QByteArray head = xml.left(200).toLower();
    if (head.contains("encoding=")) {
        if (head.contains("iso-8859-1") || head.contains("latin1")
            || head.contains("windows-1252") || head.contains("cp1252"))
            return QString::fromLatin1(xml);
        if (head.contains("utf-8"))
            return QString::fromUtf8(xml);
    }
    QStringDecoder dec(QStringConverter::Utf8);
    QString s = dec.decode(xml);
    if (dec.hasError())
        return QString::fromLatin1(xml);   // not valid UTF-8 -> treat as Latin-1
    return s;
}

} // namespace

ImportResult importFromXml(const QByteArray &xml)
{
    ImportResult res;
    const QString text = decodeXml(xml);
    QXmlStreamReader r(text);

    QMap<int, QString> categories;   // index -> name
    bool defaultsSigned = false;
    bool defaultsBigEndian = true;   // lsbfirst=0 -> big-endian

    // Two passes are awkward with a streaming reader, so parse structurally:
    // dispatch on start elements, accumulating the current object.
    while (!r.atEnd()) {
        const auto tok = r.readNext();
        if (tok != QXmlStreamReader::StartElement) continue;
        const QStringView name = r.name();

        if (name == QLatin1String("BASEOFFSET")) {
            const auto at = r.attributes();
            res.baseOffset = at.value(QStringLiteral("offset")).toString().toUInt(nullptr, 0);
        } else if (name == QLatin1String("DEFAULTS")) {
            const auto at = r.attributes();
            defaultsSigned    = at.value(QStringLiteral("signed")).toString() == QLatin1String("1");
            defaultsBigEndian = at.value(QStringLiteral("lsbfirst")).toString() != QLatin1String("1");
        } else if (name == QLatin1String("REGION")) {
            const auto at = r.attributes();
            const QString sz = at.value(QStringLiteral("size")).toString();
            res.romSize = sz.startsWith(QLatin1String("0x"))
                ? sz.mid(2).toUInt(nullptr, 16) : sz.toUInt(nullptr, 0);
        } else if (name == QLatin1String("CATEGORY")) {
            const auto at = r.attributes();
            const int idx = at.value(QStringLiteral("index")).toString().toInt(nullptr, 0);
            categories.insert(idx, at.value(QStringLiteral("name")).toString());
        } else if (name == QLatin1String("XDFCONSTANT") || name == QLatin1String("XDFTABLE")) {
            const bool isTable = name == QLatin1String("XDFTABLE");
            const QString endTag = isTable ? QStringLiteral("XDFTABLE")
                                           : QStringLiteral("XDFCONSTANT");
            MapInfo m;
            m.linkConfidence = 100;
            m.columnMajor    = false;
            m.dataSigned     = defaultsSigned;
            m.cellBigEndian  = defaultsBigEndian;
            int categoryIdx  = -1;

            Embedded zData;                // constant body or table z-axis
            bool haveZ = false;
            QString curAxis;               // "x" / "y" / "z" while inside XDFAXIS
            Embedded axisData;
            bool axisHasEmbedded = false;
            QString pendingUnits;
            double  eqA = 1.0, eqB = 0.0; bool eqLinear = false;

            auto finishAxis = [&]() {
                if (curAxis.isEmpty()) return;
                if (curAxis == QLatin1String("z")) {
                    zData = axisData; haveZ = true;
                    if (eqLinear) {
                        m.hasScaling   = true;
                        m.scaling.type = CompuMethod::Type::Linear;
                        m.scaling.linA = eqA; m.scaling.linB = eqB;
                        m.scaling.unit = pendingUnits;
                    }
                } else {
                    AxisInfo &ax = (curAxis == QLatin1String("x")) ? m.xAxis : m.yAxis;
                    if (axisHasEmbedded && axisData.hasAddress) {
                        ax.ptsAddress    = axisData.address;
                        ax.hasPtsAddress = true;
                        ax.ptsDataSize   = bytesFromBits(axisData.sizeBits);
                        ax.ptsCount      = qMax(axisData.cols, axisData.rows);
                    }
                    if (eqLinear) {
                        ax.hasScaling   = true;
                        ax.scaling.type = CompuMethod::Type::Linear;
                        ax.scaling.linA = eqA; ax.scaling.linB = eqB;
                        ax.scaling.unit = pendingUnits;
                    }
                    if (!pendingUnits.isEmpty() && pendingUnits != QLatin1String("-"))
                        ax.inputName = pendingUnits;
                }
                curAxis.clear(); axisHasEmbedded = false; pendingUnits.clear();
                eqLinear = false; eqA = 1.0; eqB = 0.0;
            };

            // Walk this object's subtree.
            while (!r.atEnd()) {
                const auto t2 = r.readNext();
                if (t2 == QXmlStreamReader::EndElement) {
                    if (r.name() == QLatin1String("XDFAXIS")) finishAxis();
                    else if (r.name() == endTag) break;
                    continue;
                }
                if (t2 != QXmlStreamReader::StartElement) continue;
                const QStringView n2 = r.name();
                if (n2 == QLatin1String("title")) {
                    m.name = r.readElementText().trimmed();
                    m.description = m.name;
                } else if (n2 == QLatin1String("CATEGORYMEM")) {
                    // category attr is 1-based (0 = none); header index is 0-based.
                    const int c = r.attributes().value(QStringLiteral("category"))
                                      .toString().toInt(nullptr, 0);
                    if (c > 0) categoryIdx = c - 1;
                } else if (n2 == QLatin1String("XDFAXIS")) {
                    curAxis = r.attributes().value(QStringLiteral("id")).toString().toLower();
                } else if (n2 == QLatin1String("EMBEDDEDDATA")) {
                    const Embedded e = readEmbedded(r.attributes());
                    if (isTable && !curAxis.isEmpty()) { axisData = e; axisHasEmbedded = true; }
                    else { zData = e; haveZ = true; }        // constant body
                } else if (n2 == QLatin1String("units")) {
                    const QString u = r.readElementText().trimmed();
                    if (!curAxis.isEmpty() || !isTable) pendingUnits = u;
                } else if (n2 == QLatin1String("MATH")) {
                    const QString eq = r.attributes().value(QStringLiteral("equation")).toString();
                    double a, b;
                    if (parseLinearEquation(eq, &a, &b)) { eqLinear = true; eqA = a; eqB = b; }
                    if (!isTable) {   // constant scaling applies directly to the map
                        if (eqLinear) {
                            m.hasScaling = true; m.scaling.type = CompuMethod::Type::Linear;
                            m.scaling.linA = a; m.scaling.linB = b;
                        }
                    }
                }
            }

            if (!haveZ || !zData.hasAddress) {
                res.warnings.append(QStringLiteral("Skipped '%1': no address").arg(m.name));
                continue;
            }

            m.rawAddress = zData.address;
            m.address    = zData.address >= res.baseOffset
                         ? zData.address - res.baseOffset : zData.address;
            m.dataSize   = bytesFromBits(zData.sizeBits);
            const int cols = qMax(1, zData.cols);
            const int rows = qMax(1, zData.rows);
            m.dimensions = { cols, rows };
            m.length     = cols * rows * m.dataSize;
            if (!isTable) {
                m.scaling.unit = pendingUnits.isEmpty() ? m.scaling.unit : pendingUnits;
                m.type = QStringLiteral("VALUE");
            } else {
                m.type = (cols > 1 && rows > 1) ? QStringLiteral("MAP")
                       : (cols > 1 || rows > 1) ? QStringLiteral("CURVE")
                                                : QStringLiteral("VALUE");
            }
            if (categoryIdx >= 0 && categories.contains(categoryIdx))
                m.folderPath = categories.value(categoryIdx);

            res.maps.append(m);
        }
    }

    if (r.hasError()) {
        res.error = QStringLiteral("XDF parse error: %1 (line %2)")
                        .arg(r.errorString()).arg(r.lineNumber());
        res.maps.clear();
    }
    return res;
}

// ── Export ───────────────────────────────────────────────────────────────────

QByteArray exportToXml(const QVector<MapInfo> &maps, const ExportOptions &opt)
{
    // Collect folder paths -> category indices (stable, insertion order).
    QVector<QString> catNames;
    QHash<QString, int> catIndex;
    for (const auto &m : maps) {
        if (m.folderPath.isEmpty()) continue;
        if (!catIndex.contains(m.folderPath)) {
            catIndex.insert(m.folderPath, catNames.size());
            catNames.append(m.folderPath);
        }
    }

    QByteArray out;
    QXmlStreamWriter w(&out);
    w.setAutoFormatting(true);
    w.writeStartDocument();
    w.writeStartElement(QStringLiteral("XDFFORMAT"));
    w.writeAttribute(QStringLiteral("version"), QStringLiteral("1.60"));

    w.writeStartElement(QStringLiteral("XDFHEADER"));
    w.writeTextElement(QStringLiteral("description"), opt.description);
    w.writeStartElement(QStringLiteral("BASEOFFSET"));
    w.writeAttribute(QStringLiteral("offset"), QString::number(opt.baseOffset));
    w.writeAttribute(QStringLiteral("subtract"), QStringLiteral("0"));
    w.writeEndElement();
    w.writeStartElement(QStringLiteral("DEFAULTS"));
    w.writeAttribute(QStringLiteral("datasizeinbits"), QStringLiteral("8"));
    w.writeAttribute(QStringLiteral("sigdigits"), QStringLiteral("2"));
    w.writeAttribute(QStringLiteral("outputtype"), QStringLiteral("1"));
    w.writeAttribute(QStringLiteral("signed"), QStringLiteral("0"));
    w.writeAttribute(QStringLiteral("lsbfirst"), opt.bigEndian ? QStringLiteral("0")
                                                               : QStringLiteral("1"));
    w.writeAttribute(QStringLiteral("float"), QStringLiteral("0"));
    w.writeEndElement();
    w.writeStartElement(QStringLiteral("REGION"));
    w.writeAttribute(QStringLiteral("type"), QStringLiteral("0xFFFFFFFF"));
    w.writeAttribute(QStringLiteral("startaddress"), QStringLiteral("0x0"));
    w.writeAttribute(QStringLiteral("size"),
        QStringLiteral("0x%1").arg(opt.romSize ? opt.romSize : 0x400000u, 0, 16));
    w.writeAttribute(QStringLiteral("regionflags"), QStringLiteral("0x0"));
    w.writeAttribute(QStringLiteral("name"), QStringLiteral("Binary File"));
    w.writeAttribute(QStringLiteral("desc"), QStringLiteral("Binary edited by this XDF"));
    w.writeEndElement();
    for (int i = 0; i < catNames.size(); i++) {
        w.writeStartElement(QStringLiteral("CATEGORY"));
        w.writeAttribute(QStringLiteral("index"), QStringLiteral("0x%1").arg(i, 0, 16));
        w.writeAttribute(QStringLiteral("name"), catNames[i]);
        w.writeEndElement();
    }
    w.writeEndElement(); // XDFHEADER

    auto writeMath = [&](const CompuMethod &cm, bool hasScaling) {
        if (!hasScaling || cm.type != CompuMethod::Type::Linear) return;
        w.writeStartElement(QStringLiteral("MATH"));
        w.writeAttribute(QStringLiteral("equation"), buildLinearEquation(cm.linA, cm.linB));
        w.writeStartElement(QStringLiteral("VAR"));
        w.writeAttribute(QStringLiteral("id"), QStringLiteral("X"));
        w.writeEndElement();
        w.writeEndElement();
    };
    auto writeEmbedded = [&](uint32_t addr, bool hasAddr, int dataSize, int rows, int cols) {
        w.writeStartElement(QStringLiteral("EMBEDDEDDATA"));
        if (hasAddr)
            w.writeAttribute(QStringLiteral("mmedaddress"),
                             QStringLiteral("0x%1").arg(addr, 0, 16));
        w.writeAttribute(QStringLiteral("mmedelementsizebits"), QString::number(dataSize * 8));
        if (rows > 1) w.writeAttribute(QStringLiteral("mmedrowcount"), QString::number(rows));
        w.writeAttribute(QStringLiteral("mmedcolcount"), QString::number(cols));
        w.writeEndElement();
    };

    int uid = 1;
    for (const auto &m : maps) {
        const int cols = qMax(1, m.dimensions.x);
        const int rows = qMax(1, m.dimensions.y);
        const bool scalar = (cols == 1 && rows == 1);
        const uint32_t addr = m.rawAddress ? m.rawAddress : m.address + opt.baseOffset;

        if (scalar) {
            w.writeStartElement(QStringLiteral("XDFCONSTANT"));
            w.writeAttribute(QStringLiteral("uniqueid"), QStringLiteral("0x%1").arg(uid++, 0, 16));
            w.writeTextElement(QStringLiteral("title"), m.name);
            if (!m.folderPath.isEmpty()) {
                w.writeStartElement(QStringLiteral("CATEGORYMEM"));
                w.writeAttribute(QStringLiteral("index"), QStringLiteral("0"));
                w.writeAttribute(QStringLiteral("category"),
                                 QString::number(catIndex.value(m.folderPath) + 1));
                w.writeEndElement();
            }
            writeEmbedded(addr, true, m.dataSize, 1, 1);
            if (m.hasScaling && !m.scaling.unit.isEmpty())
                w.writeTextElement(QStringLiteral("units"), m.scaling.unit);
            writeMath(m.scaling, m.hasScaling);
            w.writeEndElement();
            continue;
        }

        w.writeStartElement(QStringLiteral("XDFTABLE"));
        w.writeAttribute(QStringLiteral("uniqueid"), QStringLiteral("0x%1").arg(uid++, 0, 16));
        w.writeTextElement(QStringLiteral("title"), m.name);
        if (!m.folderPath.isEmpty()) {
            w.writeStartElement(QStringLiteral("CATEGORYMEM"));
            w.writeAttribute(QStringLiteral("index"), QStringLiteral("0"));
            w.writeAttribute(QStringLiteral("category"),
                             QString::number(catIndex.value(m.folderPath) + 1));
            w.writeEndElement();
        }
        auto writeTableAxis = [&](const QString &id, const AxisInfo &ax, int count) {
            w.writeStartElement(QStringLiteral("XDFAXIS"));
            w.writeAttribute(QStringLiteral("id"), id);
            writeEmbedded(ax.ptsAddress, ax.hasPtsAddress,
                          ax.ptsDataSize > 0 ? ax.ptsDataSize : 2, 1, count);
            w.writeTextElement(QStringLiteral("indexcount"), QString::number(count));
            if (ax.hasScaling && !ax.scaling.unit.isEmpty())
                w.writeTextElement(QStringLiteral("units"), ax.scaling.unit);
            writeMath(ax.scaling, ax.hasScaling);
            w.writeEndElement();
        };
        writeTableAxis(QStringLiteral("x"), m.xAxis, cols);
        writeTableAxis(QStringLiteral("y"), m.yAxis, rows);
        // z axis = the actual table data
        w.writeStartElement(QStringLiteral("XDFAXIS"));
        w.writeAttribute(QStringLiteral("id"), QStringLiteral("z"));
        writeEmbedded(addr, true, m.dataSize, rows, cols);
        if (m.hasScaling && !m.scaling.unit.isEmpty())
            w.writeTextElement(QStringLiteral("units"), m.scaling.unit);
        writeMath(m.scaling, m.hasScaling);
        w.writeEndElement();
        w.writeEndElement(); // XDFTABLE
    }

    w.writeEndElement(); // XDFFORMAT
    w.writeEndDocument();
    return out;
}

} // namespace xdf
