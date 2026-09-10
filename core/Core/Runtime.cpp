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

#include "Core/Runtime.h"

static bool s_benchmarkActive = false;
static int s_sessionId        = 0;

/**
 * @brief Whether the headless hotpath benchmark is feeding frames; the benchmark is
 *        single-threaded by construction, so a plain bool is enough.
 */
bool Core::Runtime::benchmarkActive() noexcept
{
  return s_benchmarkActive;
}

/**
 * @brief Marks the benchmark active or idle; the benchmark root refreshes the dashboard's stream
 *        flag itself after flipping this.
 */
void Core::Runtime::setBenchmarkActive(const bool active) noexcept
{
  s_benchmarkActive = active;
}

/**
 * @brief The identifier of the session this process runs; 0 is the process-default context.
 */
int Core::Runtime::sessionId() noexcept
{
  return s_sessionId;
}

/**
 * @brief Records the session identifier; the session context sets it when it is constructed.
 */
void Core::Runtime::setSessionId(const int id) noexcept
{
  s_sessionId = id;
}
