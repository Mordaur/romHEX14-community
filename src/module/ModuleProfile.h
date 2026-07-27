/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <memory>
#include "ModuleData.h"

// A ModuleProfile teaches the framework about ONE family of module dumps:
// how to recognize a file as belonging to it, and how to decode that file into
// the shared ModuleInfo. Profiles are pure read/decode — free in every build.
//
// Adding support for a new module (CAS, other BCMs, gateways, …) means writing
// a new ModuleProfile and registering it; nothing else in the app changes.
namespace module {

class ModuleProfile {
public:
    virtual ~ModuleProfile() = default;

    /// Stable id, e.g. "porsche_bcm". Used for entitlement keys and logging.
    virtual QString id() const = 0;

    /// Human label, e.g. "Porsche / Continental BCM".
    virtual QString displayName() const = 0;

    /// Cheap detection: return true if @p data looks like this module's dump.
    /// Must be conservative — false positives would mislabel unrelated files.
    virtual bool detect(const QByteArray &data) const = 0;

    /// Decode @p data into a ModuleInfo for display. Only called when
    /// detect() returned true. Never mutates @p data.
    virtual ModuleInfo parse(const QByteArray &data) const = 0;
};

using ModuleProfilePtr = std::shared_ptr<ModuleProfile>;

} // namespace module
