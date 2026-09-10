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

#include <QByteArray>

#include "Core/IO/HAL_Driver.h"

namespace IO {

/**
 * @brief A read-only observer of the raw byte stream (spec 0077): the console, the API servers
 *        and the recorders that keep a byte-level lane. The device router calls every bound tap
 *        per received chunk on the GUI thread, at chunk rate, so an implementation enqueues or
 *        appends and never blocks; the defaults let a tap observe only the lanes it cares about.
 */
class IRawByteTap {
public:
  IRawByteTap()                              = default;
  IRawByteTap(IRawByteTap&&)                 = delete;
  IRawByteTap(const IRawByteTap&)            = delete;
  IRawByteTap& operator=(IRawByteTap&&)      = delete;
  IRawByteTap& operator=(const IRawByteTap&) = delete;
  virtual ~IRawByteTap()                     = default;

  virtual void onDeviceBytes(int deviceId, const CapturedDataPtr& data) = 0;

  virtual void onConsoleBytes(int deviceId, const CapturedDataPtr& data)
  {
    Q_UNUSED(deviceId);
    Q_UNUSED(data);
  }

  virtual void onInjectedBytes(const CapturedDataPtr& data) { Q_UNUSED(data); }

  virtual void onSentBytes(int deviceId, const QByteArray& data)
  {
    Q_UNUSED(deviceId);
    Q_UNUSED(data);
  }
};

}  // namespace IO
