/*
 * Serial Studio
 * https://serial-studio.com/
 *
 * Copyright (C) 2020-2026 Alex Spataru
 *
 * This file is dual-licensed:
 *
 * - Under the GNU GPLv3 (or later) for builds that exclude Pro modules.
 * - Under the Serial Studio Commercial License for builds that include
 *   any Pro functionality.
 *
 * You must comply with the terms of one of these licenses, depending
 * on your use case.
 *
 * For GPL terms, see <https://www.gnu.org/licenses/gpl-3.0.html>
 * For commercial terms, see LICENSES/LicenseRef-SerialStudio-Commercial.txt.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-SerialStudio-Commercial
 */

#include "Core/Services.h"

#include "Core/SSAssert.h"

static Core::Services* s_services = nullptr;

/**
 * @brief Binds (or, with nullptr, unbinds) the root-owned service set; the composition root calls
 *        this once the bus and the four Core singletons exist and again with nullptr at shutdown.
 */
void Core::bindServices(Services* services) noexcept
{
  SS_ASSERT_LOG(services == nullptr || s_services == nullptr);
  s_services = services;
}

/**
 * @brief Returns the bound service set; a reach before the root bound it is a composition defect
 *        and a named fatal, the same contract the session modules' accessors keep.
 */
Core::Services& Core::services()
{
  SS_ASSERT(s_services != nullptr, qFatal("Core::services() reached before the root bound it"));
  return *s_services;
}
