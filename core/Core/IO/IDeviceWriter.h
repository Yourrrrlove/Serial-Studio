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

#pragma once

#include <QByteArray>

namespace IO {

/**
 * @brief The outbound device path a script, an action or the frame builder writes through (spec
 *        0077 T64): the plain write, and the arm/poll/disarm trio a control script's
 *        writeAndWait() drives. The connection manager implements it on the GUI thread; the
 *        composition root binds it, so no scripting file names the manager.
 */
class IDeviceWriter {
public:
  IDeviceWriter()                                = default;
  IDeviceWriter(IDeviceWriter&&)                 = delete;
  IDeviceWriter(const IDeviceWriter&)            = delete;
  IDeviceWriter& operator=(IDeviceWriter&&)      = delete;
  IDeviceWriter& operator=(const IDeviceWriter&) = delete;
  virtual ~IDeviceWriter()                       = default;

  [[nodiscard]] virtual qint64 writeDataToDevice(int deviceId, const QByteArray& data) = 0;
  [[nodiscard]] virtual qint64 writeAndArmReply(int deviceId, const QByteArray& data)  = 0;
  [[nodiscard]] virtual QByteArray pollReplyBuffer(int deviceId) const                 = 0;
  virtual void disarmReplyCapture(int deviceId)                                        = 0;
};

}  // namespace IO
