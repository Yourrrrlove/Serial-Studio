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

#include "MQTT/NotificationPayload.h"

#ifdef BUILD_COMMERCIAL

#  include <QJsonDocument>
#  include <QJsonObject>

#  include "Core/Bus/Messages.h"

/**
 * @brief The JSON the MQTT publisher forwards for one accepted notification: the same five keys
 *        the notification center's event map carried before the bus (timestamp, level, channel,
 *        title, subtitle), compact.
 */
QByteArray MQTT::notificationPayload(const Core::Bus::NotificationPosted& posted)
{
  QJsonObject event;
  event.insert(QStringLiteral("timestamp"), posted.timestampMs);
  event.insert(QStringLiteral("level"), posted.severity);
  event.insert(QStringLiteral("channel"), posted.channel);
  event.insert(QStringLiteral("title"), posted.title);
  event.insert(QStringLiteral("subtitle"), posted.text);
  return QJsonDocument(event).toJson(QJsonDocument::Compact);
}

#endif
