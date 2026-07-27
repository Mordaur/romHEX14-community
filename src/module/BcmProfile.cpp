/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "BcmProfile.h"
#include <QVector>

// Community build: display-only BCM profile. It detects the module and decodes
// the part number, VIN, supplier and the component-protection CS keys for the
// read-only info panel. The VTS (PVTS) on/off feature — its flag layout and the
// write operation — is a Pro-edition capability handled on the secure server
// and is intentionally not part of this build.

namespace module {
namespace {

constexpr int kBcmSize = 16384;
const char    kSupplierSig[] = "contiautomotive.com";

// One BCM part number and the fields we read from its DFlash. Offsets confirmed
// against real Cayenne-958 dumps. Each CS key is stored twice (primary +
// mirror); the two copies are byte-identical, which the display cross-checks.
//
// `label`/`role`/`note` are wrapped in QT_TRANSLATE_NOOP("BcmProfile", …) so
// lupdate extracts them; parse() runs each through BcmProfile::tr() at use.
struct CsKeyDef { const char *label; int off; int mirror; int len; const char *note; };

struct BcmVariant {
    const char *partNo;        // e.g. "7PP907064EB"
    const char *role;          // QT_TRANSLATE_NOOP "Front" / "Rear"
    int vinOff;                // start of the 17-char VIN
    QVector<CsKeyDef> csKeys;
};

const QVector<BcmVariant> &variants()
{
    static const QVector<BcmVariant> v = {
        {   // ── Front BCM ──
            "7PP907064EB", QT_TRANSLATE_NOOP("BcmProfile", "Front"), 0x123,
            {
                { QT_TRANSLATE_NOOP("BcmProfile", "Key-programming CS"), 0x21D, 0x211D, 16,
                  QT_TRANSLATE_NOOP("BcmProfile", "for adding keys") },
                { QT_TRANSLATE_NOOP("BcmProfile", "ECU-sync CS"),        0x22D, 0x212D, 16,
                  QT_TRANSLATE_NOOP("BcmProfile", "must match the engine ECU (EDC17)") },
            },
        },
        {   // ── Rear BCM ──
            "7PP907279AJ", QT_TRANSLATE_NOOP("BcmProfile", "Rear"), 0x123,
            {
                { QT_TRANSLATE_NOOP("BcmProfile", "CS (front-derived mix)"), 0x202, 0x2402, 16,
                  QT_TRANSLATE_NOOP("BcmProfile", "ECU-sync CS with bytes 5,6,11,12 from key-programming CS") },
            },
        },
    };
    return v;
}

bool inRange(const QByteArray &d, int off, int len)
{
    return off >= 0 && len > 0 && off + len <= d.size();
}

QString hexBytes(const QByteArray &d, int off, int len)
{
    QStringList parts;
    for (int i = 0; i < len && off + i < d.size(); ++i)
        parts << QString("%1").arg((quint8)d[off + i], 2, 16, QChar('0')).toUpper();
    return parts.join(' ');
}

QString readVin(const QByteArray &d, int off)
{
    // VIN is 17 printable chars.
    if (!inRange(d, off, 17)) return {};
    QByteArray raw = d.mid(off, 17);
    for (char c : raw)
        if (c < 32 || c > 126) return {};
    return QString::fromLatin1(raw);
}

const BcmVariant *matchVariant(const QByteArray &d)
{
    for (const BcmVariant &v : variants())
        if (d.contains(v.partNo))
            return &v;
    return nullptr;
}

} // namespace

bool BcmProfile::detect(const QByteArray &data) const
{
    return data.size() == kBcmSize
        && data.contains(kSupplierSig)
        && matchVariant(data) != nullptr;
}

ModuleInfo BcmProfile::parse(const QByteArray &data) const
{
    ModuleInfo info;
    const BcmVariant *v = matchVariant(data);
    if (!v || data.size() != kBcmSize) return info;

    info.detected     = true;
    // profileId stays English (stable key / used as an id).
    info.profileId    = QString("porsche_bcm_%1").arg(QString(v->role).toLower());
    info.moduleType   = tr("Body Control Module (BCM)");
    info.manufacturer = "Continental";                 // supplier proper noun
    info.variant      = v->partNo;
    info.role         = tr(v->role);                   // shown in the FRONT/REAR chip

    // Structured metadata for project prefill.
    const QString vinStr = readVin(data, v->vinOff);
    info.brand        = "Porsche";
    info.vehicleModel = "Cayenne 958";   // 7PP part family
    info.vin          = vinStr;
    info.ecuLabel     = QString("BCM %1 %2").arg(v->role, v->partNo);

    auto add = [&](const QString &label, const QString &value,
                   FieldKind kind, const QString &note = {}, bool ok = true) {
        info.fields.push_back({ label, value, kind, note, ok });
    };

    add(tr("Part number"), v->partNo,     FieldKind::PartNumber);
    add(tr("Supplier"),    "Continental", FieldKind::Supplier);

    const QString vin = readVin(data, v->vinOff);
    if (!vin.isEmpty())
        add(tr("VIN"), vin, FieldKind::Vin, QString("@0x%1").arg(v->vinOff, 0, 16));

    // ── Component-protection CS keys (read-only display) ───────────────────
    for (const CsKeyDef &cs : v->csKeys) {
        bool present = inRange(data, cs.off, cs.len);
        bool mMatch  = inRange(data, cs.mirror, cs.len)
                       && data.mid(cs.off, cs.len) == data.mid(cs.mirror, cs.len);
        ModuleField f;
        f.label = tr(cs.label);
        f.value = present ? hexBytes(data, cs.off, cs.len) : QStringLiteral("—");
        f.kind  = FieldKind::CsKey;
        f.note  = QString("@0x%1 (+mirror 0x%2)%3 · %4")
                      .arg(cs.off, 0, 16).arg(cs.mirror, 0, 16)
                      .arg(mMatch ? "" : "  (mirror mismatch!)").arg(tr(cs.note));
        f.ok    = present && mMatch;
        f.editable = false;                 // community build: display only
        info.fields.push_back(f);
    }

    // No VTS status field and no operations in the community build — the
    // vehicle-tracking on/off capability is Pro-only and server-side.
    return info;
}

} // namespace module
