/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <vector>
#include "ModuleProfile.h"

// Central registry of all known ModuleProfiles. The detection hook consults it
// on file load; the module info panel uses it to decode.
namespace module {

class ModuleRegistry {
public:
    static ModuleRegistry &instance();

    /// First profile whose detect() matches @p data, or nullptr.
    const ModuleProfile *detect(const QByteArray &data) const;

    /// Convenience: detect() then parse(). Returns a ModuleInfo with
    /// detected=false if nothing matched.
    ModuleInfo parse(const QByteArray &data) const;

    const std::vector<ModuleProfilePtr> &profiles() const { return m_profiles; }

private:
    ModuleRegistry();
    std::vector<ModuleProfilePtr> m_profiles;
};

} // namespace module
