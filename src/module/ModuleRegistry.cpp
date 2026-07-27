/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ModuleRegistry.h"
#include "BcmProfile.h"

namespace module {

ModuleRegistry::ModuleRegistry()
{
    // Register known module profiles here. New modules (CAS, gateways, …) are
    // added by appending their profile — no other code changes required.
    m_profiles.push_back(std::make_shared<BcmProfile>());
}

ModuleRegistry &ModuleRegistry::instance()
{
    static ModuleRegistry s;
    return s;
}

const ModuleProfile *ModuleRegistry::detect(const QByteArray &data) const
{
    for (const auto &p : m_profiles)
        if (p->detect(data))
            return p.get();
    return nullptr;
}

ModuleInfo ModuleRegistry::parse(const QByteArray &data) const
{
    if (const ModuleProfile *p = detect(data))
        return p->parse(data);
    return {};
}

} // namespace module
