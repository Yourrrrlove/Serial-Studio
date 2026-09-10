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

#include <memory>
#include <QString>

#include "Core/IO/FrameConfig.h"
#include "Core/IO/StreamConfig.h"

namespace Core::Bus {
struct ProjectStructureSnapshot;
}  // namespace Core::Bus

namespace IO {

class HAL_Driver;

/**
 * @brief Derives a source's framing, stream lane and stream-worker configuration from the
 *        retained project snapshot and the operation mode. Pure derivation: it opens nothing, owns
 * no device and keeps no state, so the spec-0044 headless bootstrap can ask it for a FrameConfig
 * with only the pinned constructor order behind it.
 */
class StreamConfigBuilder {
public:
  StreamConfigBuilder(const SerialStudio::OperationMode& operationMode,
                      const FrameConfig& source0Config,
                      const std::shared_ptr<const Core::Bus::ProjectStructureSnapshot>& project);
  StreamConfigBuilder(StreamConfigBuilder&&)                 = delete;
  StreamConfigBuilder(const StreamConfigBuilder&)            = delete;
  StreamConfigBuilder& operator=(StreamConfigBuilder&&)      = delete;
  StreamConfigBuilder& operator=(const StreamConfigBuilder&) = delete;

  [[nodiscard]] QString streamLane(int deviceId) const;
  [[nodiscard]] FrameConfig frameConfig(int deviceId) const;
  [[nodiscard]] StreamConfig streamConfig(int deviceId, HAL_Driver* driver) const;

private:
  void appendProjectChannels(int deviceId, StreamConfig& config) const;

  static void appendQuickPlotChannels(StreamConfig& config);

private:
  const SerialStudio::OperationMode& m_operationMode;
  const FrameConfig& m_source0Config;
  const std::shared_ptr<const Core::Bus::ProjectStructureSnapshot>& m_project;
};

}  // namespace IO
