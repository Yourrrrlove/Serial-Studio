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
#include <QMap>

namespace IO {

/**
 * @brief The path a replay player feeds pre-built payloads through (spec 0077 T65): the device
 *        router takes them exactly like received bytes, so every tap sees the replay. The
 *        connection manager implements it and the root binds it into the players; a payload is
 *        a synchronous replay-rate call, which is why this is an interface and not a topic.
 */
class IPayloadInjector {
public:
  IPayloadInjector()                                   = default;
  IPayloadInjector(IPayloadInjector&&)                 = delete;
  IPayloadInjector(const IPayloadInjector&)            = delete;
  IPayloadInjector& operator=(IPayloadInjector&&)      = delete;
  IPayloadInjector& operator=(const IPayloadInjector&) = delete;
  virtual ~IPayloadInjector()                          = default;

  virtual void processPayload(const QByteArray& payload)                              = 0;
  virtual void processMultiSourcePayload(const QByteArray& fullPayload,
                                         const QMap<int, QByteArray>& sourcePayloads) = 0;
};

}  // namespace IO
