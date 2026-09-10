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

#include <memory>
#include <QSignalSpy>
#include <QTest>
#include <type_traits>
#include <vector>

#include "Core/DataModel/DataBlock.h"
#include "Core/DataModel/FrameConsumer.h"
#include "Core/DataModel/IBlockSink.h"

// The sink contract the block publisher binds by interface (spec 0077): a sink takes blocks
// through the base pointer, reports whether it is consuming, and announces every edge of that
// verdict, because the publisher's cached any-async-sink flag is derived from nothing else.

/**
 * @brief A sink that counts what it is handed and flips its activity on request.
 */
class CountingSink : public DataModel::IBlockSink {
  Q_OBJECT

public:
  void ingestBlock(const DataModel::DataBlockPtr& block) override
  {
    ++blocks;
    last = block;
  }

  [[nodiscard]] bool sinkActive() const noexcept override { return m_active; }

  void setActive(bool active)
  {
    if (m_active == active)
      return;

    m_active = active;
    Q_EMIT sinkActivityChanged();
  }

  int blocks = 0;
  DataModel::DataBlockPtr last;

private:
  bool m_active = false;
};

/**
 * @brief A worker that writes nowhere; the consumer under test never starts it.
 */
class NullWorker : public DataModel::FrameConsumerWorker<DataModel::DataBlockPtr> {
public:
  using FrameConsumerWorker::FrameConsumerWorker;

protected:
  void processItems(const std::vector<DataModel::DataBlockPtr>&) override {}

  void closeResources() override {}

  [[nodiscard]] bool isResourceOpen() const override { return false; }
};

/**
 * @brief The shape every recorder shares: a FrameConsumer that enqueues the blocks it ingests.
 */
class TinyConsumer : public DataModel::FrameConsumer<DataModel::DataBlockPtr> {
  Q_OBJECT

public:
  void ingestBlock(const DataModel::DataBlockPtr& block) override
  {
    ++ingested;
    enqueueData(block);
  }

  [[nodiscard]] bool sinkActive() const noexcept override { return consumerEnabled(); }

  [[nodiscard]] size_t pending() const { return m_pendingQueue.size_approx(); }

  int ingested = 0;

protected:
  DataModel::FrameConsumerWorkerBase* createWorker() override
  {
    return new NullWorker(&m_pendingQueue, &m_consumerEnabled, &m_queueSize);
  }
};

/**
 * @brief Builds a one-sample block for @p sourceId.
 */
static DataModel::DataBlockPtr makeBlock(int sourceId)
{
  auto block      = std::make_shared<DataModel::DataBlock>();
  block->sourceId = sourceId;
  block->samples  = 1;
  return block;
}

class TstBlockSink : public QObject {
  Q_OBJECT

private slots:
  void blocksReachTheSinkThroughTheBase();
  void activityEdgesAnnounceOnce();
  void frameConsumersAreBlockSinks();
};

/**
 * @brief The publisher only ever holds an IBlockSink*: ingestion through the base reaches the
 *        concrete sink with the same block.
 */
void TstBlockSink::blocksReachTheSinkThroughTheBase()
{
  CountingSink sink;
  DataModel::IBlockSink* base = &sink;

  const auto first  = makeBlock(0);
  const auto second = makeBlock(1);
  base->ingestBlock(first);
  base->ingestBlock(second);

  QCOMPARE(sink.blocks, 2);
  QCOMPARE(sink.last, second);
  QVERIFY(!base->sinkActive());
}

/**
 * @brief sinkActivityChanged fires once per real transition and never for a repeated value, so
 *        the publisher re-derives its cached flag exactly when the verdict moved.
 */
void TstBlockSink::activityEdgesAnnounceOnce()
{
  CountingSink sink;
  QSignalSpy spy(&sink, &DataModel::IBlockSink::sinkActivityChanged);

  sink.setActive(true);
  sink.setActive(true);
  QCOMPARE(spy.count(), 1);
  QVERIFY(sink.sinkActive());

  sink.setActive(false);
  QCOMPARE(spy.count(), 2);
  QVERIFY(!sink.sinkActive());
}

/**
 * @brief Every FrameConsumer is a block sink: the recorders inherit the contract, and a block
 *        ingested through the base lands in the consumer's lock-free queue.
 */
void TstBlockSink::frameConsumersAreBlockSinks()
{
  static_assert(
    std::is_base_of_v<DataModel::IBlockSink, DataModel::FrameConsumer<DataModel::DataBlockPtr>>);

  TinyConsumer consumer;
  DataModel::IBlockSink* base = &consumer;
  QVERIFY(base->sinkActive());

  base->ingestBlock(makeBlock(0));
  base->ingestBlock(makeBlock(0));
  base->ingestBlock(makeBlock(0));
  QCOMPARE(consumer.ingested, 3);
  QCOMPARE(consumer.pending(), static_cast<size_t>(3));

  consumer.setConsumerEnabled(false);
  QVERIFY(!base->sinkActive());
}

QTEST_GUILESS_MAIN(TstBlockSink)
#include "tst_block_sink.moc"
