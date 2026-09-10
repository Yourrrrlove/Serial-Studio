/*
 * Serial Studio
 * https://serial-studio.com/
 *
 * Copyright (C) 2020-2025 Alex Spataru
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

#include "Core/License.h"

#include <atomic>

//--------------------------------------------------------------------------------------------------
// State
//--------------------------------------------------------------------------------------------------

static std::atomic<bool> s_activated{false};
static std::atomic<quint8> s_tier{0};
static std::atomic<bool> s_trialExpired{false};

//--------------------------------------------------------------------------------------------------
// Readers
//--------------------------------------------------------------------------------------------------

/**
 * @brief Whether a valid commercial entitlement (paid or trial) is installed.
 */
bool Core::License::activated() noexcept
{
  return s_activated.load(std::memory_order_relaxed);
}

/**
 * @brief The feature tier the root derived from the installed token (0 when none).
 */
quint8 Core::License::tier() noexcept
{
  return s_tier.load(std::memory_order_relaxed);
}

/**
 * @brief Whether the trial period ended on this machine.
 */
bool Core::License::trialExpired() noexcept
{
  return s_trialExpired.load(std::memory_order_relaxed);
}

//--------------------------------------------------------------------------------------------------
// Writer (composition root only)
//--------------------------------------------------------------------------------------------------

/**
 * @brief Publishes the licensing facts; called by the root after the licensing block constructs
 *        and on every activatedChanged transition, never from a library.
 */
void Core::License::set(const bool activated, const quint8 tier, const bool trialExpired) noexcept
{
  s_tier.store(tier, std::memory_order_relaxed);
  s_trialExpired.store(trialExpired, std::memory_order_relaxed);
  s_activated.store(activated, std::memory_order_release);
}
