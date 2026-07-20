/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDialog>
#include <QByteArray>
#include <QString>

class QLineEdit;
class QLabel;
class QTableWidget;
class QPushButton;

// Interactive VAG .frf / .sgo / .odx extraction. Lets the user point at the
// XOR key (VW frf.key) and, for encrypted blocks, an AES key/IV file, runs the
// FrfExtractor pipeline, lists the resulting flash blocks, and hands a decoded
// block to the host to open as a project (or saves it to a .bin).
class FrfImportDlg : public QDialog {
    Q_OBJECT
public:
    explicit FrfImportDlg(const QString &containerPath, QWidget *parent = nullptr);

signals:
    // Emitted when the user chooses to open a decoded block as a project.
    void openRomRequested(const QByteArray &romBytes, const QString &suggestedName);

private:
    void browseXorKey();
    void browseAesKey();
    void runExtract();
    void openSelected();
    void saveSelected();
    int  selectedDecodedRow() const;

    QString    m_path;
    QByteArray m_container;
    QByteArray m_lastDecoded;   // cache for the currently-selected block

    QLabel      *m_fileLabel   = nullptr;
    QLineEdit   *m_xorKeyEdit  = nullptr;
    QLineEdit   *m_aesKeyEdit  = nullptr;
    QLabel      *m_statusLabel = nullptr;
    QTableWidget*m_table       = nullptr;
    QPushButton *m_btnOpen     = nullptr;
    QPushButton *m_btnSave     = nullptr;

    // Decoded block bytes kept per table row (parallel to m_table rows).
    QVector<QByteArray> m_rowData;
    QVector<QString>    m_rowName;
};
