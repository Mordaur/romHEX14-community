/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ModuleProfile.h"
#include <QCoreApplication>   // Q_DECLARE_TR_FUNCTIONS

// Porsche / Continental Body Control Module (BCM) DFlash profile.
//
// Handles the 16 KB data-flash dumps (front + rear) of the Cayenne-family BCM.
// Community build: reads and displays part number, VIN, supplier and the
// component-protection CS keys (read-only). The VTS (PVTS) on/off feature is a
// Pro-edition capability and is not part of this build. Layout is a data-driven
// variant table so more BCM part numbers are one row each.
namespace module {

class BcmProfile : public ModuleProfile {
    Q_DECLARE_TR_FUNCTIONS(BcmProfile)   // user-facing field/status strings are translatable
public:
    QString    id() const override { return "porsche_bcm"; }
    QString    displayName() const override { return "Porsche / Continental BCM"; }
    bool       detect(const QByteArray &data) const override;
    ModuleInfo parse(const QByteArray &data) const override;
};

} // namespace module
