/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "valuesearchdlg.h"
#include "appconfig.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <cmath>
#include <cstring>

ValueSearchDlg::ValueSearchDlg(const QByteArray &rom, ByteOrder defaultOrder, QWidget *parent)
    : QDialog(parent)
    , m_rom(rom)
{
    setWindowTitle(tr("Find Value in ROM"));
    resize(560, 480);

    const AppColors &c = AppConfig::instance().colors;
    setStyleSheet("QDialog{background:" + c.uiBg.name() + ";}"
                  "QLabel{color:" + c.uiText.name() + ";}"
                  "QCheckBox{color:" + c.uiText.name() + ";}");

    const QString fieldStyle =
        "QLineEdit,QComboBox,QDoubleSpinBox{background:" + c.inputBg.name() +
        ";color:" + c.uiText.name() + ";border:1px solid " + c.inputBorder.name() +
        ";border-radius:4px;padding:3px 6px;}"
        "QComboBox QAbstractItemView{background:" + c.inputBg.name() +
        ";color:" + c.uiText.name() + ";selection-background-color:" + c.uiAccent.name() + ";}";

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 12);
    root->setSpacing(10);

    auto *grid = new QGridLayout();
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(6);
    int r = 0;

    auto label = [&](const QString &t){ auto *l = new QLabel(t); return l; };

    m_valueEdit = new QLineEdit();
    m_valueEdit->setStyleSheet(fieldStyle);
    m_valueEdit->setPlaceholderText(tr("e.g. 6500  — physical value to find"));
    grid->addWidget(label(tr("Value:")),   r, 0);
    grid->addWidget(m_valueEdit,           r, 1, 1, 3); ++r;

    m_typeCombo = new QComboBox();
    m_typeCombo->setStyleSheet(fieldStyle);
    m_typeCombo->addItem(tr("8-bit"),  1);
    m_typeCombo->addItem(tr("16-bit"), 2);
    m_typeCombo->addItem(tr("32-bit"), 4);
    m_typeCombo->addItem(tr("Float 32"), 5);   // 5 = float sentinel
    m_typeCombo->setCurrentIndex(1);
    grid->addWidget(label(tr("Data type:")), r, 0);
    grid->addWidget(m_typeCombo,             r, 1);

    m_orderCombo = new QComboBox();
    m_orderCombo->setStyleSheet(fieldStyle);
    m_orderCombo->addItem(tr("Big Endian"),    int(ByteOrder::BigEndian));
    m_orderCombo->addItem(tr("Little Endian"), int(ByteOrder::LittleEndian));
    m_orderCombo->setCurrentIndex(defaultOrder == ByteOrder::LittleEndian ? 1 : 0);
    grid->addWidget(label(tr("Byte order:")), r, 2);
    grid->addWidget(m_orderCombo,             r, 3); ++r;

    m_signedChk = new QCheckBox(tr("Signed"));
    m_alignedChk = new QCheckBox(tr("Aligned only"));
    m_alignedChk->setChecked(true);
    m_alignedChk->setToolTip(tr("Only match at offsets that are a multiple of the data width"));
    grid->addWidget(m_signedChk,  r, 1);
    grid->addWidget(m_alignedChk, r, 2, 1, 2); ++r;

    m_tolSpin = new QDoubleSpinBox();
    m_tolSpin->setStyleSheet(fieldStyle);
    m_tolSpin->setRange(0.0, 1e9);
    m_tolSpin->setDecimals(3);
    m_tolSpin->setValue(0.0);
    grid->addWidget(label(tr("Tolerance ±:")), r, 0);
    grid->addWidget(m_tolSpin,                 r, 1); ++r;

    m_factorSpin = new QDoubleSpinBox();
    m_factorSpin->setStyleSheet(fieldStyle);
    m_factorSpin->setRange(-1e9, 1e9);
    m_factorSpin->setDecimals(6);
    m_factorSpin->setValue(1.0);
    m_offsetSpin = new QDoubleSpinBox();
    m_offsetSpin->setStyleSheet(fieldStyle);
    m_offsetSpin->setRange(-1e9, 1e9);
    m_offsetSpin->setDecimals(6);
    m_offsetSpin->setValue(0.0);
    grid->addWidget(label(tr("Scale factor:")), r, 0);
    grid->addWidget(m_factorSpin,               r, 1);
    grid->addWidget(label(tr("Offset:")),       r, 2);
    grid->addWidget(m_offsetSpin,               r, 3); ++r;

    root->addLayout(grid);

    auto *hint = new QLabel(tr(
        "Searches every address whose stored value equals the target. Scaling is "
        "applied as physical = raw × factor + offset before comparison — leave at "
        "1 / 0 to match the raw number."));
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#8b949e; font-size:8pt;");
    root->addWidget(hint);

    auto *btnSearch = new QPushButton(tr("Search"));
    btnSearch->setStyleSheet(
        "QPushButton{background:" + c.uiAccent.name() + ";color:#fff;border:none;"
        "border-radius:4px;padding:5px 16px;font-weight:bold;}"
        "QPushButton:hover{background:" + c.uiAccent.lighter(120).name() + ";}");
    connect(btnSearch, &QPushButton::clicked, this, &ValueSearchDlg::runSearch);
    connect(m_valueEdit, &QLineEdit::returnPressed, this, &ValueSearchDlg::runSearch);
    auto *searchRow = new QHBoxLayout();
    searchRow->addStretch();
    searchRow->addWidget(btnSearch);
    root->addLayout(searchRow);

    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color:#8b949e; font-size:8pt;");
    root->addWidget(m_statusLabel);

    m_table = new QTableWidget(0, 3);
    m_table->setHorizontalHeaderLabels({tr("Address"), tr("Raw"), tr("Physical")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->setStyleSheet(
        "QTableWidget{background:" + c.inputBg.name() + ";color:" + c.uiText.name() +
        ";gridline-color:" + c.uiBorder.name() + ";border:1px solid " + c.uiBorder.name() + ";}"
        "QHeaderView::section{background:" + c.uiPanel.name() + ";color:" + c.uiTextDim.name() +
        ";border:none;padding:4px;}");
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int, int) { goToSelected(); });
    root->addWidget(m_table, 1);

    auto *btnRow = new QHBoxLayout();
    auto *btnClose = new QPushButton(tr("Close"));
    connect(btnClose, &QPushButton::clicked, this, &QDialog::close);
    auto *btnMap = new QPushButton(tr("Create Map Here"));
    connect(btnMap, &QPushButton::clicked, this, &ValueSearchDlg::createMapAtSelected);
    auto *btnGo = new QPushButton(tr("Go To Address"));
    connect(btnGo, &QPushButton::clicked, this, &ValueSearchDlg::goToSelected);
    btnRow->addWidget(btnClose);
    btnRow->addStretch();
    btnRow->addWidget(btnMap);
    btnRow->addWidget(btnGo);
    root->addLayout(btnRow);
}

void ValueSearchDlg::runSearch()
{
    m_table->setRowCount(0);
    m_rowAddr.clear();

    bool ok = false;
    const double target = m_valueEdit->text().trimmed().toDouble(&ok);
    if (!ok) { m_statusLabel->setText(tr("Enter a numeric value to search for.")); return; }

    const int typeSel = m_typeCombo->currentData().toInt();
    const bool isFloat = (typeSel == 5);
    const int cellSize = isFloat ? 4 : typeSel;
    const ByteOrder bo = ByteOrder(m_orderCombo->currentData().toInt());
    const bool isSigned = m_signedChk->isChecked();
    const int step = m_alignedChk->isChecked() ? cellSize : 1;
    const double tol = m_tolSpin->value();
    const double factor = m_factorSpin->value();
    const double offset = m_offsetSpin->value();

    const auto *raw = reinterpret_cast<const uint8_t*>(m_rom.constData());
    const int len = m_rom.size();
    const int last = len - cellSize;

    const int kMaxResults = 10000;
    int found = 0, shown = 0;

    for (int off = 0; off <= last; off += step) {
        double phys;
        if (isFloat) {
            uint32_t bits = readRomValue(raw, len, uint32_t(off), 4, bo);
            float fv; std::memcpy(&fv, &bits, 4);
            if (!std::isfinite(fv)) continue;
            phys = double(fv) * factor + offset;
        } else {
            double rv = readRomValueAsDouble(raw, len, uint32_t(off), cellSize, bo, isSigned);
            phys = rv * factor + offset;
        }
        if (std::fabs(phys - target) <= tol) {
            ++found;
            if (shown < kMaxResults) {
                const int row = m_table->rowCount();
                m_table->insertRow(row);
                m_table->setItem(row, 0, new QTableWidgetItem(
                    QStringLiteral("0x%1").arg(uint32_t(off), 0, 16).toUpper()));
                uint32_t rawv = readRomValue(raw, len, uint32_t(off), cellSize, bo);
                m_table->setItem(row, 1, new QTableWidgetItem(
                    isFloat ? QStringLiteral("0x%1").arg(rawv, 0, 16)
                            : QString::number(readRomValueAsDouble(raw, len, uint32_t(off), cellSize, bo, isSigned), 'g', 8)));
                m_table->setItem(row, 2, new QTableWidgetItem(QString::number(phys, 'g', 8)));
                m_rowAddr.append(uint32_t(off));
                ++shown;
            }
        }
    }

    m_table->resizeColumnsToContents();
    m_table->horizontalHeader()->setStretchLastSection(true);
    QString msg = tr("%n match(es)", nullptr, found);
    if (found > shown) msg += tr(" — showing first %1").arg(shown);
    m_statusLabel->setText(msg);
    if (shown > 0) m_table->selectRow(0);
}

int ValueSearchDlg::selectedAddressRow() const
{
    const auto sel = m_table->selectionModel()->selectedRows();
    if (sel.isEmpty()) return -1;
    const int r = sel.first().row();
    return (r >= 0 && r < m_rowAddr.size()) ? r : -1;
}

void ValueSearchDlg::goToSelected()
{
    const int r = selectedAddressRow();
    if (r < 0) return;
    emit goToAddressRequested(m_rowAddr[r]);
}

void ValueSearchDlg::createMapAtSelected()
{
    const int r = selectedAddressRow();
    if (r < 0) return;
    const int typeSel = m_typeCombo->currentData().toInt();
    const int cellSize = (typeSel == 5) ? 4 : typeSel;
    emit createMapRequested(m_rowAddr[r], cellSize);
}
