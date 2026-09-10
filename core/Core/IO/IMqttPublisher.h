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
#include <QString>

namespace IO {

/**
 * @brief The one verb a script may aim at the project's MQTT publisher (spec 0077): publish a
 *        payload on a topic and get the packet id back. The table-API bridges hold this interface,
 *        bound by the composition root, so the scripting layer never names the publisher.
 */
class IMqttPublisher {
public:
  IMqttPublisher()                                 = default;
  IMqttPublisher(IMqttPublisher&&)                 = delete;
  IMqttPublisher(const IMqttPublisher&)            = delete;
  IMqttPublisher& operator=(IMqttPublisher&&)      = delete;
  IMqttPublisher& operator=(const IMqttPublisher&) = delete;
  virtual ~IMqttPublisher()                        = default;

  [[nodiscard]] virtual qint64 mqttPublish(const QString& topic,
                                           const QByteArray& payload,
                                           int qos,
                                           bool retain) = 0;
};

}  // namespace IO
