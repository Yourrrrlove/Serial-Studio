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

#include "Core/Crypto/MachineKey.h"

#include <atomic>

#include "Core/SSAssert.h"

static std::atomic<quint64> s_machineKey{0};

/**
 * @brief The published machine key; zero until the root published it, which a vault must not
 *        seal with.
 */
quint64 Core::Crypto::machineKey() noexcept
{
  const quint64 key = s_machineKey.load(std::memory_order_acquire);
  SS_ASSERT_LOG(key != 0);
  return key;
}

/**
 * @brief Publishes the machine key; called by the composition root once, before any vault seals.
 */
void Core::Crypto::setMachineKey(const quint64 key) noexcept
{
  s_machineKey.store(key, std::memory_order_release);
}
