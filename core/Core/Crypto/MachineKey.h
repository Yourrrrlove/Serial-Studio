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

#include <QtGlobal>

/**
 * @file MachineKey.h
 * @brief The machine-specific cipher key the credential vaults seal secrets with. The value is
 *        derived by the licensing fingerprint in the composition root, which publishes it here
 *        once at startup (spec 0077); Core never computes it and a library never reaches the
 *        licensing module for it.
 */

namespace Core::Crypto {
[[nodiscard]] quint64 machineKey() noexcept;

void setMachineKey(quint64 key) noexcept;
}  // namespace Core::Crypto
