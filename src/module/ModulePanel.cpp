/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ModulePanel.h"
#include "ToggleSwitch.h"
#include "../appconfig.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>

namespace {

QString groupedHex(const QString &spacedBytes)
{
    const QString raw = QString(spacedBytes).remove(' ').toUpper();
    QString out;
    for (int i = 0; i < raw.size(); i += 4) {
        if (!out.isEmpty()) out += ' ';
        out += raw.mid(i, 4);
    }
    return out;
}
QString normHex(const QString &s) { return QString(s).remove(' ').toUpper(); }

QString rgba(const QColor &c, double a)
{
    return QString("rgba(%1,%2,%3,%4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(a);
}

} // namespace

ModulePanel::ModulePanel(QWidget *parent)
    : QWidget(parent)
{
    m_root = new QVBoxLayout(this);
    m_root->setContentsMargins(0, 0, 0, 0);
    m_root->setSpacing(0);
    rebuild();
}

void ModulePanel::setModuleInfo(const module::ModuleInfo &info)
{
    m_info = info;
    rebuild();
}

void ModulePanel::applyEdits()
{
    QVector<module::ModulePatch> patches;
    for (const auto &pr : m_editors) {
        const module::ModuleField &f = m_info.fields[pr.first];
        QLineEdit *edit = pr.second;
        const QByteArray bytes = QByteArray::fromHex(normHex(edit->text()).toLatin1());
        if (bytes.size() != f.writeLen) continue;
        if (normHex(edit->text()) == normHex(f.value)) continue;
        for (int off : f.writeOffsets)
            patches.push_back({ off, bytes });
    }
    if (!patches.isEmpty())
        emit writeRequested(patches);
}

void ModulePanel::rebuild()
{
    using namespace module;
    m_editors.clear();
    if (m_content) { m_content->deleteLater(); m_content = nullptr; }

    const AppColors &c = AppConfig::instance().colors;
    const int fs = qMax(9, font().pointSize() > 0 ? font().pointSize() : 12);
    const QString accent = c.uiAccent.name();

    // ── Empty state ────────────────────────────────────────────────────
    if (!m_info.detected) {
        auto *ph = new QLabel(tr("◍\n\nOpen a supported control-module dump\n"
                                 "to reveal its data here."));
        ph->setAlignment(Qt::AlignCenter);
        ph->setStyleSheet(QString("color:%1; padding:28px; font-size:%2pt;")
                              .arg(c.uiTextDim.name()).arg(fs));
        m_content = ph;
        m_root->addWidget(ph, 1);
        return;
    }

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("QScrollArea{border:none;background:transparent;}"
                          "QScrollBar:vertical{background:transparent;width:9px;margin:2px;}"
                          "QScrollBar::handle:vertical{background:" + c.uiBorder.name() + ";border-radius:4px;min-height:24px;}"
                          "QScrollBar::add-line,QScrollBar::sub-line{height:0;}");
    auto *body = new QWidget;
    // Panel-wide vertical gradient for depth.
    body->setStyleSheet(QString(
        "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %1,stop:0.35 %2,stop:1 %2);")
        .arg(c.uiPanel.name(), c.uiBg.name()));
    scroll->setWidget(body);
    m_content = scroll;
    m_root->addWidget(scroll, 1);

    auto *root = new QVBoxLayout(body);
    root->setContentsMargins(16, 16, 16, 18);
    root->setSpacing(0);
    auto space = [&](int px){ root->addSpacing(px); };

    // ── Header: title + FRONT/REAR chip + part number ──────────────────
    {
        auto *hrow = new QHBoxLayout(); hrow->setSpacing(9);
        auto *title = new QLabel(m_info.moduleType);
        title->setStyleSheet(QString("color:%1;font-size:%2pt;font-weight:800;"
                                     "background:transparent;letter-spacing:0.3px;")
                                 .arg(c.uiText.name()).arg(fs + 4));
        hrow->addWidget(title);
        hrow->addStretch();
        auto *chip = new QLabel(m_info.role.toUpper());
        chip->setStyleSheet(QString(
            "color:%1;background:%2;border:1px solid %3;border-radius:9px;"
            "padding:2px 10px;font-size:%4pt;font-weight:800;letter-spacing:1px;")
            .arg(accent, rgba(c.uiAccent, 0.14), rgba(c.uiAccent, 0.5)).arg(fs - 3));
        hrow->addWidget(chip, 0, Qt::AlignVCenter);
        root->addLayout(hrow);
        space(3);
        auto *sub = new QLabel(QString("%1  ·  %2").arg(m_info.manufacturer, m_info.variant));
        sub->setStyleSheet(QString("color:%1;font-size:%2pt;font-family:Consolas,monospace;"
                                   "letter-spacing:0.5px;background:transparent;")
                               .arg(c.uiTextDim.name()).arg(fs - 2));
        root->addWidget(sub);
        space(16);
    }

    // ── VTS hero ───────────────────────────────────────────────────────
    const ModuleField *statusF = nullptr;
    for (const ModuleField &f : m_info.fields)
        if (f.kind == FieldKind::Status) { statusF = &f; break; }

    if (statusF) {
        const QColor sc = statusF->statusOn ? QColor(0x3f, 0xb9, 0x50)   // active → green
                                            : QColor(0xf0, 0xa0, 0x20);  // disabled → amber
        auto *hero = new QFrame;
        hero->setStyleSheet(QString(
            "QFrame#hero{background:qlineargradient(x1:0,y1:0,x2:1,y2:1,"
            "stop:0 %1, stop:1 %2);border:1px solid %3;border-left:4px solid %4;"
            "border-radius:12px;}")
            .arg(rgba(sc, 0.16), rgba(sc, 0.03), rgba(sc, 0.35), sc.name()));
        hero->setObjectName("hero");
        auto *hl = new QVBoxLayout(hero);
        hl->setContentsMargins(16, 13, 16, 15);
        hl->setSpacing(0);

        auto *cap = new QLabel(tr("Vehicle tracking system").toUpper());
        cap->setStyleSheet(QString("color:%1;font-size:%2pt;font-weight:800;letter-spacing:2px;"
                                   "border:none;background:transparent;")
                               .arg(c.uiTextDim.name()).arg(fs - 3));
        hl->addWidget(cap);
        hl->addSpacing(10);

        auto *midRow = new QHBoxLayout(); midRow->setSpacing(10);
        auto *st = new QLabel(QString("<span style='color:%1;font-size:%2pt;'>●</span>"
                                      "&nbsp;&nbsp;<span style='color:%1;font-size:%3pt;font-weight:800;"
                                      "letter-spacing:1px;'>%4</span>")
                                  .arg(sc.name()).arg(fs).arg(fs + 6)
                                  .arg(statusF->value.toUpper()));
        st->setTextFormat(Qt::RichText);
        st->setStyleSheet("border:none;background:transparent;");
        midRow->addWidget(st, 0, Qt::AlignVCenter);
        midRow->addStretch();

        auto *toggle = new ToggleSwitch();
        toggle->setOnColor(QColor(0x3f, 0xb9, 0x50));
        toggle->setChecked(statusF->statusOn, /*animate*/false);
        int th = qMax(24, fs + 12);
        toggle->setFixedSize(th * 2, th);
        if (!m_info.operations.isEmpty()) {
            const QString profileId = m_info.profileId;
            const QString opId = m_info.operations.first().id;
            connect(toggle, &ToggleSwitch::toggled, this,
                    [this, profileId, opId](bool) { emit operationRequested(profileId, opId); });
        }
        midRow->addWidget(toggle, 0, Qt::AlignVCenter);
        hl->addLayout(midRow);

        hl->addSpacing(11);
        auto *lock = new QLabel(tr("🔒  Pro subscription · applied on the secure server"));
        lock->setWordWrap(true);
        lock->setStyleSheet(QString("color:%1;font-size:%2pt;border:none;background:transparent;")
                                .arg(c.uiTextDim.name()).arg(fs - 3));
        hl->addWidget(lock);

        root->addWidget(hero);
        space(20);
    }

    // ── Section header: accent bar + label ─────────────────────────────
    auto sectionHeader = [&](const QString &text) {
        auto *rowW = new QWidget;
        auto *hb = new QHBoxLayout(rowW);
        hb->setContentsMargins(0, 0, 0, 0); hb->setSpacing(9);
        auto *bar = new QFrame;
        bar->setFixedSize(3, fs + 1);
        bar->setStyleSheet(QString("background:%1;border-radius:1px;").arg(accent));
        hb->addWidget(bar);
        auto *lbl = new QLabel(text.toUpper());
        lbl->setStyleSheet(QString("color:%1;font-size:%2pt;font-weight:800;letter-spacing:2px;"
                                   "background:transparent;")
                               .arg(c.uiText.name()).arg(fs - 2));
        hb->addWidget(lbl);
        hb->addStretch();
        root->addWidget(rowW);
        space(11);
    };

    // ── IDENTITY ───────────────────────────────────────────────────────
    QVector<int> idFields, keyFields;
    for (int i = 0; i < m_info.fields.size(); ++i) {
        const FieldKind k = m_info.fields[i].kind;
        if (k == FieldKind::Status) continue;
        if (k == FieldKind::CsKey || k == FieldKind::Hex) keyFields.push_back(i);
        else idFields.push_back(i);
    }

    if (!idFields.isEmpty()) {
        sectionHeader(tr("Vehicle"));
        for (int i : idFields) {
            const ModuleField &f = m_info.fields[i];
            auto *lbl = new QLabel(f.label);
            lbl->setStyleSheet(QString("color:%1;font-size:%2pt;background:transparent;")
                                   .arg(c.uiTextDim.name()).arg(fs - 1));
            if (f.kind == FieldKind::Vin) {
                // VIN styled like a stamped plate.
                root->addWidget(lbl); space(4);
                auto *plate = new QLabel(f.value);
                plate->setTextInteractionFlags(Qt::TextSelectableByMouse);
                plate->setStyleSheet(QString(
                    "color:%1;background:%2;border:1px solid %3;border-radius:6px;"
                    "padding:7px 10px;font-family:Consolas,monospace;font-size:%4pt;"
                    "font-weight:700;letter-spacing:2px;")
                    .arg(c.uiText.name(), rgba(c.uiText, 0.04), c.uiBorder.name()).arg(fs));
                root->addWidget(plate);
                space(13);
            } else {
                auto *rowW = new QHBoxLayout(); rowW->setSpacing(10);
                rowW->addWidget(lbl); rowW->addStretch();
                auto *v = new QLabel(f.value);
                v->setTextInteractionFlags(Qt::TextSelectableByMouse);
                v->setStyleSheet(QString("color:%1;font-size:%2pt;font-weight:700;"
                                         "font-family:Consolas,monospace;background:transparent;")
                                     .arg(c.uiText.name()).arg(fs));
                rowW->addWidget(v);
                root->addLayout(rowW);
                space(12);
            }
        }
        space(8);
    }

    // ── COMPONENT PROTECTION (keys) ────────────────────────────────────
    if (!keyFields.isEmpty()) {
        sectionHeader(tr("🔑 Component protection"));
        QPushButton *applyBtn = nullptr;

        for (int i : keyFields) {
            const ModuleField &f = m_info.fields[i];

            // A "vault" card per key.
            auto *card = new QFrame;
            card->setStyleSheet(QString("QFrame{background:%1;border:1px solid %2;border-radius:8px;}")
                                    .arg(rgba(c.uiText, 0.03), c.uiBorder.name()));
            auto *cl = new QVBoxLayout(card);
            cl->setContentsMargins(12, 9, 12, 11); cl->setSpacing(6);

            auto *lr = new QHBoxLayout(); lr->setSpacing(6);
            auto *klbl = new QLabel(f.label);
            klbl->setStyleSheet(QString("color:%1;font-size:%2pt;font-weight:700;"
                                        "border:none;background:transparent;")
                                    .arg(c.uiText.name()).arg(fs - 1));
            lr->addWidget(klbl);
            if (!f.ok) {
                auto *warn = new QLabel("⚠ " + tr("mirror mismatch"));
                warn->setStyleSheet(QString("color:#e0705a;font-size:%1pt;border:none;background:transparent;")
                                        .arg(fs - 3));
                lr->addWidget(warn);
            }
            lr->addStretch();
            cl->addLayout(lr);

            if (f.editable) {
                auto *edit = new QLineEdit(groupedHex(f.value));
                edit->setStyleSheet(QString(
                    "QLineEdit{background:%1;border:1px solid %2;border-radius:5px;"
                    "color:%3;font-family:Consolas,monospace;font-size:%4pt;padding:5px 7px;"
                    "letter-spacing:0.4px;selection-background-color:%5;}"
                    "QLineEdit:focus{border:1px solid %6;background:%7;}")
                    .arg(c.uiBg.name(), c.uiBorder.name(), c.uiText.name())
                    .arg(fs - 1).arg(accent, accent, rgba(c.uiAccent, 0.06)));
                edit->setCursorPosition(0);
                cl->addWidget(edit);
                m_editors.push_back({ i, edit });
            } else {
                auto *v = new QLabel(groupedHex(f.value));
                v->setWordWrap(true);
                v->setTextInteractionFlags(Qt::TextSelectableByMouse);
                v->setStyleSheet(QString("color:%1;font-family:Consolas,monospace;font-size:%2pt;"
                                         "letter-spacing:0.4px;border:none;background:transparent;")
                                     .arg(c.uiText.name()).arg(fs - 1));
                cl->addWidget(v);
            }
            const QString desc = f.note.section(QStringLiteral("· "), 1);
            if (!desc.isEmpty()) {
                auto *note = new QLabel(desc);
                note->setWordWrap(true);
                note->setStyleSheet(QString("color:%1;font-size:%2pt;border:none;background:transparent;")
                                        .arg(c.uiTextDim.name()).arg(fs - 3));
                cl->addWidget(note);
            }
            root->addWidget(card);
            space(9);
        }

        if (!m_editors.isEmpty()) {
            space(2);
            applyBtn = new QPushButton("  " + tr("Write keys to ROM") + "  ");
            applyBtn->setEnabled(false);
            applyBtn->setCursor(Qt::PointingHandCursor);
            applyBtn->setStyleSheet(QString(
                "QPushButton{background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %1,stop:1 %2);"
                "color:#fff;border:none;border-radius:7px;padding:8px 16px;font-size:%3pt;font-weight:800;"
                "letter-spacing:0.5px;}"
                "QPushButton:hover{background:%4;}"
                "QPushButton:disabled{background:%5;color:%6;}")
                .arg(c.uiAccent.lighter(115).name(), c.uiAccent.name()).arg(fs - 1)
                .arg(c.uiAccent.lighter(125).name(), rgba(c.uiText, 0.05), c.uiTextDim.name()));
            connect(applyBtn, &QPushButton::clicked, this, [this]() { applyEdits(); });
            auto reeval = [this, applyBtn]() {
                bool dirty = false;
                for (const auto &pr : m_editors)
                    if (normHex(pr.second->text()) != normHex(m_info.fields[pr.first].value)) dirty = true;
                applyBtn->setEnabled(dirty);
            };
            for (const auto &pr : m_editors)
                connect(pr.second, &QLineEdit::textChanged, this, [reeval](const QString &){ reeval(); });
            root->addWidget(applyBtn, 0, Qt::AlignLeft);
        }
    }

    root->addStretch(1);
}
