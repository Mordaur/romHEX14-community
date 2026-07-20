/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDialog>
#include <QByteArray>
#include "romdata.h"

class QLineEdit;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;
class QLabel;
class QTableWidget;

// WinOLS-style numeric value search across the whole ROM. The user enters a
// value and a data interpretation (width / byte order / signedness / scaling)
// and gets every address whose stored value matches within a tolerance — the
// manual counterpart to automatic map detection. Double-clicking a hit jumps
// the active view to that address.
class ValueSearchDlg : public QDialog {
    Q_OBJECT
public:
    ValueSearchDlg(const QByteArray &rom, ByteOrder defaultOrder, QWidget *parent = nullptr);

signals:
    void goToAddressRequested(uint32_t address);
    void createMapRequested(uint32_t address, int cellSize);

private:
    void runSearch();
    void goToSelected();
    void createMapAtSelected();
    int  selectedAddressRow() const;

    QByteArray m_rom;

    QLineEdit      *m_valueEdit  = nullptr;
    QComboBox      *m_typeCombo  = nullptr;   // 8/16/32-bit / float32
    QComboBox      *m_orderCombo = nullptr;
    QCheckBox      *m_signedChk  = nullptr;
    QCheckBox      *m_alignedChk = nullptr;
    QDoubleSpinBox *m_tolSpin    = nullptr;
    QDoubleSpinBox *m_factorSpin = nullptr;   // physical = raw * factor + offset
    QDoubleSpinBox *m_offsetSpin = nullptr;
    QLabel         *m_statusLabel = nullptr;
    QTableWidget   *m_table      = nullptr;

    // Parallel to table rows: the match address.
    QVector<uint32_t> m_rowAddr;
};
