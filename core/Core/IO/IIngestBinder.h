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

#include <vector>

#include "Core/IO/FrameConfig.h"
#include "Core/IO/HAL_Driver.h"
#include "Core/IO/LinkStats.h"
#include "Core/IO/StreamConfig.h"

namespace IO {

/**
 * @brief The seam between the device layer and the acquisition pipeline (spec 0077): the connection
 *        manager hands a driver's byte stream to the pipeline here and never names the reader, the
 *        host thread or the frame builder. Readers are created behind attach(), recreated behind
 *        reconfigure() (never locked: FrameReader is single-producer) and retired behind detach().
 */
class IIngestBinder {
public:
  IIngestBinder()                                = default;
  IIngestBinder(IIngestBinder&&)                 = delete;
  IIngestBinder(const IIngestBinder&)            = delete;
  IIngestBinder& operator=(IIngestBinder&&)      = delete;
  IIngestBinder& operator=(const IIngestBinder&) = delete;
  virtual ~IIngestBinder()                       = default;

  virtual void attach(int deviceId, HAL_Driver* driver, const FrameConfig& config) = 0;
  virtual void reconfigure(int deviceId, const FrameConfig& config)                = 0;
  virtual void detach(int deviceId)                                                = 0;

  virtual void rebuildStreams(const std::vector<StreamAttachment>& sources,
                              bool paused,
                              bool connected) = 0;
  virtual void setStreamPaused(bool paused)   = 0;
  virtual void publishStreamTemplates()       = 0;
  virtual void detachStreams()                = 0;

  virtual void injectPayload(int sourceId, const CapturedDataPtr& payload) = 0;
  virtual void resetQuickPlotHeaders()                                     = 0;

  [[nodiscard]] virtual LinkStats linkStats() const = 0;
};

}  // namespace IO
