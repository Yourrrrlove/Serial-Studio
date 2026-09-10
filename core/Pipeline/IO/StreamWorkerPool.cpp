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

#include "IO/StreamWorkerPool.h"

#include <QSet>

#include "Core/SSAssert.h"
#include "DataModel/FrameBuilder.h"

//--------------------------------------------------------------------------------------------------
// Constructor & destructor
//--------------------------------------------------------------------------------------------------

/**
 * @brief Binds the pool to the pipeline host's operation-mode mirror; the block consumer arrives
 *        with bind(), after the pinned construction order (ctor-edge rule, spec 0001).
 */
IO::StreamWorkerPool::StreamWorkerPool(const std::atomic<int>& operationMode)
  : m_operationMode(operationMode), m_frameBuilder(nullptr)
{}

/**
 * @brief Joins every worker still running. ModuleManager stops the pool explicitly before the
 *        session context releases anything, so this is the last-resort path only.
 */
IO::StreamWorkerPool::~StreamWorkerPool()
{
  stop();
}

/**
 * @brief Adopts the single block consumer every worker publishes into.
 */
void IO::StreamWorkerPool::bind(DataModel::FrameBuilder& frameBuilder)
{
  SS_ASSERT_LOG(m_frameBuilder == nullptr || m_frameBuilder == &frameBuilder);
  m_frameBuilder = &frameBuilder;
}

//--------------------------------------------------------------------------------------------------
// Lifecycle
//--------------------------------------------------------------------------------------------------

/**
 * @brief Rebuilds the per-source workers from the attachments the connection layer derived (one
 *        per source whose lane is on and binds a channel); old workers stop first (bounded join).
 */
void IO::StreamWorkerPool::rebuild(const std::vector<StreamAttachment>& sources,
                                   bool paused,
                                   bool connected)
{
  stop();

  SS_ASSERT(m_frameBuilder != nullptr, return);

  for (const auto& source : sources) {
    SS_ASSERT_LOG(source.driver != nullptr);
    if (!source.driver || source.config.datasets.empty())
      continue;

    auto worker =
      std::make_unique<StreamWorker>(source.driver, source.config, m_frameBuilder, nullptr);
    worker->setPaused(paused);
    wireSinks(*worker);
    m_workers.push_back(std::move(worker));
  }

  refreshExportFlags();

  QSet<int> streamSourceIds;
  for (const auto& worker : m_workers)
    if (worker)
      streamSourceIds.insert(worker->sourceId());

  m_frameBuilder->setStreamSourceIds(streamSourceIds);

  if (!m_workers.empty() && connected)
    publishTemplates();
}

/**
 * @brief Stops and destroys every worker (idempotent; called on rebuilds and at quit from
 *        ModuleManager::stopFrameConsumerWorkers before SessionContext::shutdown).
 */
void IO::StreamWorkerPool::stop()
{
  for (auto& worker : m_workers)
    if (worker)
      worker->stop();

  m_workers.clear();
}

/**
 * @brief Mirrors the session pause onto every worker's pause atomic.
 */
void IO::StreamWorkerPool::setPaused(bool paused)
{
  for (auto& worker : m_workers)
    if (worker)
      worker->setPaused(paused);
}

/**
 * @brief Returns the live workers (GUI thread only; Dashboard drains their display rings on the
 *        display tick).
 */
const std::vector<std::unique_ptr<IO::StreamWorker>>& IO::StreamWorkerPool::workers() const noexcept
{
  return m_workers;
}

//--------------------------------------------------------------------------------------------------
// Sink wiring & publication
//--------------------------------------------------------------------------------------------------

/**
 * @brief Connects one worker's block-rate outputs, both to the FrameBuilder on the pipeline
 *        thread (spec 0055 D8): blocks join the frame lane's publish tail so the pipeline stays
 *        the SINGLE producer for every sink, and latest values keep feeding the data-table store
 *        whose single writer is that same thread.
 */
void IO::StreamWorkerPool::wireSinks(StreamWorker& worker) const
{
  SS_ASSERT(m_frameBuilder != nullptr, return);

  auto* processor = worker.processor();
  SS_ASSERT(processor != nullptr, return);

  QObject::connect(processor,
                   &IO::StreamProcessor::blockReady,
                   m_frameBuilder,
                   &DataModel::FrameBuilder::ingestStreamBlock,
                   Qt::QueuedConnection);

  QObject::connect(processor,
                   &IO::StreamProcessor::latestValuesReady,
                   m_frameBuilder,
                   &DataModel::FrameBuilder::ingestStreamValues,
                   Qt::QueuedConnection);
}

/**
 * @brief Re-derives the FrameBuilder's cached any-async-sink flag after a rebuild. Since spec
 *        0055 D8 both lanes publish through that one flag, so this no longer pushes a per-worker
 *        export gate; the sinks themselves announce their edges to the builder (spec 0077).
 */
void IO::StreamWorkerPool::refreshExportFlags() const
{
  SS_ASSERT(m_frameBuilder != nullptr, return);

  m_frameBuilder->refreshAsyncSinks();
}

/**
 * @brief Publishes the dashboard structure for every stream source so widget models build
 *        before display updates arrive: per-source template frames in ProjectFile mode, the
 *        synthesized audio frame in QuickPlot (spec 0051 T25). Nothing to publish without workers.
 */
void IO::StreamWorkerPool::publishTemplates() const
{
  if (m_workers.empty() || !m_frameBuilder)
    return;

  auto* frameBuilder = m_frameBuilder;
  const auto mode =
    static_cast<SerialStudio::OperationMode>(m_operationMode.load(std::memory_order_relaxed));
  if (mode == SerialStudio::ProjectFile) {
    for (const auto& worker : m_workers) {
      const int sourceId = worker->sourceId();
      frameBuilder->invokeOnBuilderThread(
        [frameBuilder, sourceId] { frameBuilder->publishSourceTemplate(sourceId); });
    }

    return;
  }

  for (const auto& worker : m_workers) {
    const int channels = worker->config().channels;
    frameBuilder->invokeOnBuilderThread(
      [frameBuilder, channels] { frameBuilder->publishQuickPlotAudioTemplate(channels); });
  }
}
