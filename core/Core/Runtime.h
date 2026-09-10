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

#pragma once

/**
 * @file Runtime.h
 * @brief Process-wide runtime facts a library reads as a plain flag (spec 0077): the headless
 *        hotpath benchmark marks itself active here so the dashboard accepts frames without a
 *        device, and the dashboard reads it inside its own constructor, so this is a static bool
 *        and never a construction.
 */

namespace Core::Runtime {
[[nodiscard]] bool benchmarkActive() noexcept;
void setBenchmarkActive(bool active) noexcept;
[[nodiscard]] int sessionId() noexcept;
void setSessionId(int id) noexcept;
}  // namespace Core::Runtime
