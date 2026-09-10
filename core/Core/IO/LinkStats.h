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

namespace IO {

/**
 * @brief Link counters summed across every attached frame reader, sampled once per second by the
 *        problem center and the Historian. Readers are recreated on connect and reconfigure, so a
 *        decrease means a reset and consumers work on deltas (spec 0033: pulled, never pushed).
 */
struct LinkStats {
  quint64 bytesIn;
  quint64 droppedFrames;
  quint64 overflowBytes;
  quint64 checksumErrors;
  quint64 framesExtracted;
};

}  // namespace IO
