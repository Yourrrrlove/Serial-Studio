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

#include "Core/IO/HAL_Driver.h"

namespace IO {

/**
 * @brief A read-only observer of the delimited frames the readers extract (spec 0077). The
 *        pipeline host calls the one bound tap per frame on the processing thread, through a
 *        pointer hoisted out of the drain loop, so an implementation enqueues into its own SPSC
 *        queue and never allocates, blocks or touches GUI state.
 */
class IRawFrameTap {
public:
  IRawFrameTap()                               = default;
  IRawFrameTap(IRawFrameTap&&)                 = delete;
  IRawFrameTap(const IRawFrameTap&)            = delete;
  IRawFrameTap& operator=(IRawFrameTap&&)      = delete;
  IRawFrameTap& operator=(const IRawFrameTap&) = delete;
  virtual ~IRawFrameTap()                      = default;

  virtual void onRawFrame(int deviceId, const CapturedDataPtr& frame) = 0;
};

}  // namespace IO
