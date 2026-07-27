/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QWidget>
#include <QVector>
#include "ModuleData.h"

class QVBoxLayout;
class QLineEdit;

// Pinned dock panel that shows a decoded control-module dump (BCM, CAS, …).
//
// Auto-populated when a supported module file is detected; stays docked. Data
// display is free in every build. Write operations (VTS toggle, key funcs) are
// buttons that emit operationRequested() — the host runs them server-side
// behind the Pro paywall.
class ModulePanel : public QWidget {
    Q_OBJECT
public:
    explicit ModulePanel(QWidget *parent = nullptr);

    /// Rebuild the panel for @p info. A default (detected=false) ModuleInfo
    /// shows the empty placeholder.
    void setModuleInfo(const module::ModuleInfo &info);
    bool hasModule() const { return m_info.detected; }
    QString profileId() const { return m_info.profileId; }

signals:
    /// User asked to run a write operation; host executes it server-side.
    void operationRequested(const QString &profileId, const QString &opId);

    /// User edited one or more fields and hit Apply; host writes the byte
    /// patches into the ROM (local, free — edits are just structured hex).
    void writeRequested(const QVector<module::ModulePatch> &patches);

private:
    void rebuild();
    void applyEdits();

    module::ModuleInfo m_info;
    QVBoxLayout       *m_root    = nullptr;
    QWidget           *m_content = nullptr;

    // Editable fields for the current build: (field index, editor).
    QVector<QPair<int, QLineEdit *>> m_editors;
};
