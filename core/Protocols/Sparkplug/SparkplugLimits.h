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

#include "Protocols/OpcUa/OpcUaWire.h"
#include "Protocols/Sparkplug/SparkplugPayload.h"

namespace IO::Drivers {

/**
 * @brief Fixed caps for the Sparkplug B session state, shared by the host-application session
 *        (Devices) and the edge-node publisher (Storage). Broker traffic decides how many nodes,
 *        devices, metrics and out-of-order messages arrive, so every container that grows has a
 *        ceiling here and refuses past it (counted, never resized on demand). The slot ceiling is
 *        the OPC UA wire ceiling because the delta encoder that consumes the slot table is the
 *        same one.
 */
namespace SparkplugLimits {
inline constexpr int kMaxSlots            = OpcUaWire::kMaxTags;
inline constexpr int kMaxPreBirthMessages = 256;
inline constexpr int kMaxNodes            = 256;
inline constexpr int kMaxDevicesPerNode   = 64;
inline constexpr int kMaxTopicElements    = 5;
inline constexpr quint64 kSeqModulus      = 256;

// Namespace element every Sparkplug B v1.0 topic starts with
inline constexpr const char* kNamespace = "spBv1.0";

// Synthetic per-edge-node metric carrying the birth/death state (R5)
inline constexpr const char* kOnlineMetric = "Online";

// The identity cap is the encoder's own truncation point, so nothing is retained that cannot ship
static_assert(SparkplugB::kMaxIdentityBytes == OpcUaWire::kMaxStringBytes);
}  // namespace SparkplugLimits

}  // namespace IO::Drivers
