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

#include <atomic>
#include <memory>
#include <vector>

#include "Core/IO/StreamConfig.h"
#include "IO/StreamWorker.h"

namespace DataModel {
class FrameBuilder;
}  // namespace DataModel

namespace IO {

/**
 * @brief Owns the live dense-lane workers, one per source the connection layer attached (spec
 *        0077 derives the lane and the configuration there). Everything a worker emits goes to the
 *        FrameBuilder QUEUED (spec 0055 D8) so the pipeline thread stays the single producer for
 *        every sink; the pool never routes a block itself and never caps a source's rate.
 */
class StreamWorkerPool {
public:
  explicit StreamWorkerPool(const std::atomic<int>& operationMode);
  ~StreamWorkerPool();
  StreamWorkerPool(StreamWorkerPool&&)                 = delete;
  StreamWorkerPool(const StreamWorkerPool&)            = delete;
  StreamWorkerPool& operator=(StreamWorkerPool&&)      = delete;
  StreamWorkerPool& operator=(const StreamWorkerPool&) = delete;

  void bind(DataModel::FrameBuilder& frameBuilder);
  void stop();
  void setPaused(bool paused);
  void publishTemplates() const;
  void rebuild(const std::vector<StreamAttachment>& sources, bool paused, bool connected);

  [[nodiscard]] const std::vector<std::unique_ptr<StreamWorker>>& workers() const noexcept;

private:
  void wireSinks(StreamWorker& worker) const;
  void refreshExportFlags() const;

private:
  const std::atomic<int>& m_operationMode;
  DataModel::FrameBuilder* m_frameBuilder;
  std::vector<std::unique_ptr<StreamWorker>> m_workers;
};

}  // namespace IO
