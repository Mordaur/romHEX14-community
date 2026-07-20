/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "frfimportdlg.h"
#include "appconfig.h"
#include "io/vag/FrfExtractor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>

FrfImportDlg::FrfImportDlg(const QString &containerPath, QWidget *parent)
    : QDialog(parent)
    , m_path(containerPath)
{
    setWindowTitle(tr("Import VAG FRF / ODX"));
    setModal(true);
    resize(620, 460);

    QFile f(m_path);
    if (f.open(QIODevice::ReadOnly)) m_container = f.readAll();

    const AppColors &c = AppConfig::instance().colors;
    setStyleSheet("QDialog{background:" + c.uiBg.name() + ";}"
                  "QLabel{color:" + c.uiText.name() + ";}");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 12);
    root->setSpacing(10);

    m_fileLabel = new QLabel(tr("Container: %1  (%2 bytes)")
                                 .arg(QFileInfo(m_path).fileName())
                                 .arg(m_container.size()));
    m_fileLabel->setStyleSheet("font-weight:bold;");
    root->addWidget(m_fileLabel);

    auto *note = new QLabel(tr(
        "Keys are never shipped with romHEX14. For a raw .frf, supply the VW "
        "frf.key (XOR). Encrypted blocks additionally need a 32-byte AES file "
        "(16-byte key followed by 16-byte IV)."));
    note->setWordWrap(true);
    note->setStyleSheet("color:#8b949e; font-size:8pt;");
    root->addWidget(note);

    auto *grid = new QGridLayout();
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(6);

    const QString editStyle =
        "QLineEdit{background:" + c.inputBg.name() + ";color:" + c.uiText.name() +
        ";border:1px solid " + c.inputBorder.name() + ";border-radius:4px;padding:3px 6px;}";

    auto addKeyRow = [&](int row, const QString &label, QLineEdit *&edit,
                         void (FrfImportDlg::*slot)()) {
        auto *lbl = new QLabel(label);
        edit = new QLineEdit();
        edit->setReadOnly(true);
        edit->setStyleSheet(editStyle);
        auto *btn = new QPushButton(tr("Browse…"));
        connect(btn, &QPushButton::clicked, this, slot);
        grid->addWidget(lbl,  row, 0);
        grid->addWidget(edit, row, 1);
        grid->addWidget(btn,  row, 2);
    };
    addKeyRow(0, tr("frf.key (XOR):"), m_xorKeyEdit, &FrfImportDlg::browseXorKey);
    addKeyRow(1, tr("AES key+IV:"),    m_aesKeyEdit, &FrfImportDlg::browseAesKey);
    root->addLayout(grid);

    // Restore remembered key paths.
    QSettings s("CT14", "RX14");
    m_xorKeyEdit->setText(s.value("vag/frfKeyPath").toString());
    m_aesKeyEdit->setText(s.value("vag/aesKeyPath").toString());

    auto *btnExtract = new QPushButton(tr("Extract"));
    btnExtract->setStyleSheet(
        "QPushButton{background:" + c.uiAccent.name() + ";color:#fff;border:none;"
        "border-radius:4px;padding:5px 16px;font-weight:bold;}"
        "QPushButton:hover{background:" + c.uiAccent.lighter(120).name() + ";}");
    connect(btnExtract, &QPushButton::clicked, this, &FrfImportDlg::runExtract);
    auto *extractRow = new QHBoxLayout();
    extractRow->addStretch();
    extractRow->addWidget(btnExtract);
    root->addLayout(extractRow);

    m_statusLabel = new QLabel();
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("color:#8b949e; font-size:8pt;");
    root->addWidget(m_statusLabel);

    m_table = new QTableWidget(0, 4);
    m_table->setHorizontalHeaderLabels(
        {tr("Block"), tr("Address"), tr("Size"), tr("Status")});
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
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this] {
        const int r = selectedDecodedRow();
        m_btnOpen->setEnabled(r >= 0);
        m_btnSave->setEnabled(r >= 0);
    });
    root->addWidget(m_table, 1);

    auto *btnRow = new QHBoxLayout();
    m_btnSave = new QPushButton(tr("Save Block…"));
    m_btnOpen = new QPushButton(tr("Open as Project"));
    m_btnSave->setEnabled(false);
    m_btnOpen->setEnabled(false);
    connect(m_btnSave, &QPushButton::clicked, this, &FrfImportDlg::saveSelected);
    connect(m_btnOpen, &QPushButton::clicked, this, &FrfImportDlg::openSelected);
    auto *btnClose = new QPushButton(tr("Close"));
    connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(btnClose);
    btnRow->addStretch();
    btnRow->addWidget(m_btnSave);
    btnRow->addWidget(m_btnOpen);
    root->addLayout(btnRow);
}

void FrfImportDlg::browseXorKey()
{
    QString p = QFileDialog::getOpenFileName(this, tr("Select frf.key"), {},
        tr("Key files (*.key *.bin);;All files (*)"));
    if (p.isEmpty()) return;
    m_xorKeyEdit->setText(p);
    QSettings("CT14", "RX14").setValue("vag/frfKeyPath", p);
}

void FrfImportDlg::browseAesKey()
{
    QString p = QFileDialog::getOpenFileName(this, tr("Select AES key+IV file (32 bytes)"), {},
        tr("Key files (*.key *.bin);;All files (*)"));
    if (p.isEmpty()) return;
    m_aesKeyEdit->setText(p);
    QSettings("CT14", "RX14").setValue("vag/aesKeyPath", p);
}

void FrfImportDlg::runExtract()
{
    m_table->setRowCount(0);
    m_rowData.clear();
    m_rowName.clear();
    m_btnOpen->setEnabled(false);
    m_btnSave->setEnabled(false);

    if (m_container.isEmpty()) {
        m_statusLabel->setText(tr("Could not read the container file."));
        return;
    }

    auto readFile = [](const QString &p) -> QByteArray {
        if (p.isEmpty()) return {};
        QFile f(p);
        return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
    };

    vag::FrfExtractor::Keys keys;
    keys.xorKey = readFile(m_xorKeyEdit->text());
    const QByteArray aes = readFile(m_aesKeyEdit->text());
    if (aes.size() >= 32) { keys.aesKey = aes.left(16); keys.aesIv = aes.mid(16, 16); }

    const vag::FrfResult res = vag::FrfExtractor::extract(m_container, keys);

    if (!res.ok) {
        m_statusLabel->setText(tr("Extraction failed at stage “%1”: %2")
                                   .arg(res.stage, res.error));
        return;
    }

    int decoded = 0;
    for (const vag::FrfBlock &b : res.blocks) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(b.id));
        m_table->setItem(row, 1, new QTableWidgetItem(
            b.address ? QStringLiteral("0x%1").arg(b.address, 0, 16) : QStringLiteral("—")));
        m_table->setItem(row, 2, new QTableWidgetItem(
            b.decoded ? tr("%1 bytes").arg(b.data.size()) : QStringLiteral("—")));
        QString status = b.decoded
            ? tr("Decoded%1%2")
                  .arg(b.encrypted ? tr(" · AES") : QString())
                  .arg(b.compressed ? tr(" · LZSS") : QString())
            : (b.needsKey ? tr("Needs AES key") : (b.note.isEmpty() ? tr("Skipped") : b.note));
        m_table->setItem(row, 3, new QTableWidgetItem(status));

        m_rowData.append(b.decoded ? b.data : QByteArray());
        m_rowName.append(b.id.isEmpty() ? QStringLiteral("block%1").arg(row) : b.id);
        if (b.decoded) ++decoded;
    }
    m_table->resizeColumnsToContents();
    m_table->horizontalHeader()->setStretchLastSection(true);

    QString summary = tr("%n block(s) found", nullptr, res.blocks.size());
    summary += tr(" · %n decoded", nullptr, decoded);
    if (res.anyNeedsKey)
        summary += tr(" · some blocks need an AES key/IV");
    m_statusLabel->setText(summary);

    // Auto-select the primary (largest) decoded block.
    const int primary = res.primaryBlockIndex();
    if (primary >= 0) m_table->selectRow(primary);
}

int FrfImportDlg::selectedDecodedRow() const
{
    const auto sel = m_table->selectionModel()->selectedRows();
    if (sel.isEmpty()) return -1;
    const int r = sel.first().row();
    if (r < 0 || r >= m_rowData.size() || m_rowData[r].isEmpty()) return -1;
    return r;
}

void FrfImportDlg::openSelected()
{
    const int r = selectedDecodedRow();
    if (r < 0) return;
    const QString base = QFileInfo(m_path).completeBaseName() + "_" + m_rowName[r];
    emit openRomRequested(m_rowData[r], base);
    accept();
}

void FrfImportDlg::saveSelected()
{
    const int r = selectedDecodedRow();
    if (r < 0) return;
    const QString suggested = QFileInfo(m_path).completeBaseName() + "_" + m_rowName[r] + ".bin";
    QString p = QFileDialog::getSaveFileName(this, tr("Save Block"), suggested,
        tr("ROM files (*.bin);;All files (*)"));
    if (p.isEmpty()) return;
    QFile f(p);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(m_rowData[r]);
        m_statusLabel->setText(tr("Saved %1 bytes to %2").arg(m_rowData[r].size()).arg(p));
    } else {
        QMessageBox::warning(this, tr("Save Block"), tr("Could not write the file."));
    }
}
