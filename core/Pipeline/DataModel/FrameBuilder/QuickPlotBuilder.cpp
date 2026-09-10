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

#include "DataModel/FrameBuilder/QuickPlotBuilder.h"

#include <utility>
#include <vector>

#include "Core/Bus/MessageBus.h"
#include "Core/Bus/Messages.h"
#include "Core/Services.h"
#include "Core/SSAssert.h"
#include "IO/PipelineHost.h"

//--------------------------------------------------------------------------------------------------
// Retained facts
//--------------------------------------------------------------------------------------------------

/**
 * @brief The bus type of the primary source as the connection manager last retained it, or UART
 *        before any link state was published.
 */
[[nodiscard]] static int activeBusType()
{
  const auto* bus = &Core::services().bus;
  const auto link = bus ? bus->latest<Core::Bus::ConnectionStateChanged>() : nullptr;
  return link ? link->busType : static_cast<int>(SerialStudio::BusType::UART);
}

//--------------------------------------------------------------------------------------------------
// Construction
//--------------------------------------------------------------------------------------------------

/**
 * @brief Binds the builder to the frame builder's cached operation mode, so the mode guards below
 *        read the same value the parse lanes do instead of a second copy that could drift.
 */
DataModel::QuickPlotBuilder::QuickPlotBuilder(const SerialStudio::OperationMode& operationMode)
  : m_operationMode(operationMode), m_hasHeader(false)
{}

//--------------------------------------------------------------------------------------------------
// Header registration
//--------------------------------------------------------------------------------------------------

/**
 * @brief Registers Quick Plot channel headers, or clears them when @p headers is empty.
 */
void DataModel::QuickPlotBuilder::setHeaders(const QStringList& headers)
{
  if (!headers.isEmpty()) {
    m_hasHeader    = true;
    m_channelNames = headers;
  } else {
    m_hasHeader = false;
    m_channelNames.clear();
  }
}

//--------------------------------------------------------------------------------------------------
// Frame construction
//--------------------------------------------------------------------------------------------------

/**
 * @brief Builds the synthetic source row that anchors a QuickPlot frame. Always returns a
 *        non-null title so downstream exporters bound to NOT NULL columns don't reject the row.
 */
DataModel::Source DataModel::QuickPlotBuilder::makeSource() const
{
  DataModel::Source src;
  src.sourceId = 0;
  src.title    = tr("Device A");
  src.busType  = activeBusType();
  return src;
}

/**
 * @brief Rebuilds the Quick Plot frame structure when the channel count changes. The caller
 *        invalidates the frame pool first: a rebuilt structure must never share a generation with
 *        the slots staged from the old one.
 */
void DataModel::QuickPlotBuilder::build(const QStringList& channels)
{
  SS_ASSERT(!channels.isEmpty(), return);
  SS_ASSERT(m_operationMode == SerialStudio::QuickPlot, return);

#ifdef BUILD_COMMERCIAL
  if (activeBusType() == static_cast<int>(SerialStudio::BusType::Audio)) {
    buildAudio(channels);
    return;
  }
#endif

  int idx = 1;
  std::vector<DataModel::Dataset> datasets;
  datasets.reserve(channels.count());
  for (const auto& channel : std::as_const(channels)) {
    DataModel::Dataset dataset;
    dataset.groupId   = 0;
    dataset.datasetId = idx - 1;
    dataset.uniqueId  = dataset_unique_id(0, 0, idx - 1);
    dataset.index     = idx;
    dataset.plt       = false;
    dataset.value     = channel;

    if (m_hasHeader && idx > 0 && idx - 1 < static_cast<int>(m_channelNames.size()))
      dataset.title = m_channelNames[idx - 1];
    else
      dataset.title = tr("Channel %1").arg(idx);

    dataset.numericValue = SerialStudio::toDouble(dataset.value, &dataset.isNumeric);
    datasets.push_back(dataset);

    ++idx;
  }

  clear_frame(m_frame);
  m_frame.title = tr("Quick Plot");
  m_frame.sources.push_back(makeSource());

  DataModel::Group datagrid;
  datagrid.groupId  = 0;
  datagrid.uniqueId = runtime_group_unique_id(0);
  datagrid.datasets = datasets;
  datagrid.title    = tr("Quick Plot Data");
  datagrid.widget   = QStringLiteral("datagrid");
  for (size_t i = 0; i < datagrid.datasets.size(); ++i)
    datagrid.datasets[i].plt = true;

  m_frame.groups.push_back(datagrid);

  if (datasets.size() > 1) {
    DataModel::Group multiplot;
    multiplot.groupId  = 1;
    multiplot.uniqueId = runtime_group_unique_id(1);
    multiplot.datasets = datasets;
    multiplot.title    = tr("Multi-Plot");
    multiplot.widget   = QStringLiteral("multiplot");
    for (size_t i = 0; i < multiplot.datasets.size(); ++i) {
      multiplot.datasets[i].groupId  = 1;
      multiplot.datasets[i].uniqueId = dataset_unique_id(0, 1, static_cast<int>(i));
    }

    m_frame.groups.push_back(multiplot);
  }

  finalize_frame(m_frame);
}

#ifdef BUILD_COMMERCIAL
/**
 * @brief The miniaudio ma_format ordinals the AudioCaptureFormat topic carries, mirrored here so
 *        the pipeline sizes the audio lane without the driver's header.
 */
enum AudioSampleFormat : int {
  kAudioFormatU8  = 1,
  kAudioFormatS16 = 2,
  kAudioFormatS24 = 3,
  kAudioFormatS32 = 4,
  kAudioFormatF32 = 5,
};

/**
 * @brief Returns the numeric display range of a miniaudio capture format, or the normalized
 * -1..1 range when the driver publishes normalized samples.
 */
static void audioFormatRange(const int fmt, bool normalized, double& minValue, double& maxValue)
{
  if (normalized) {
    minValue = -1.0;
    maxValue = 1.0;
    return;
  }

  switch (fmt) {
    case kAudioFormatU8:
      maxValue = 255;
      minValue = 0;
      break;
    case kAudioFormatS16:
      maxValue = 32767;
      minValue = -32768;
      break;
    case kAudioFormatS24:
      maxValue = 8388607;
      minValue = -8388608;
      break;
    case kAudioFormatS32:
      maxValue = 2147483647;
      minValue = -2147483648;
      break;
    case kAudioFormatF32:
      maxValue = 1.0;
      minValue = -1.0;
      break;
    default:
      maxValue = 1.0;
      minValue = 0.0;
      break;
  }
}
#endif

/**
 * @brief Builds an audio-specific Quick Plot frame with FFT configuration.
 */
void DataModel::QuickPlotBuilder::buildAudio(const QStringList& channels)
{
  SS_ASSERT(!channels.isEmpty(), return);
  SS_ASSERT(m_operationMode == SerialStudio::QuickPlot, return);

#ifdef BUILD_COMMERCIAL
  int format      = 0;
  int sampleRate  = 0;
  bool haveAudio  = false;
  bool normalized = false;
  IO::PipelineHost::runOnGuiThreadBlocking([&] {
    const auto* bus  = &Core::services().bus;
    const auto audio = bus ? bus->latest<Core::Bus::AudioCaptureFormat>() : nullptr;
    if (!audio)
      return;

    format     = audio->format;
    sampleRate = audio->sampleRate;
    normalized = audio->normalized;
    haveAudio  = true;
  });

  if (!haveAudio)
    return;

  double maxValue = 1.0;
  double minValue = 0.0;
  audioFormatRange(format, normalized, minValue, maxValue);

  const int targetSamples = static_cast<int>(sampleRate * 0.05);
  int fftSamples          = 256;
  while (fftSamples < targetSamples && fftSamples < 8192)
    fftSamples *= 2;

  const bool multipleChannels = channels.count() > 1;
  int index                   = 1;
  std::vector<DataModel::Dataset> datasets;
  datasets.reserve(channels.count());
  for (const auto& channel : std::as_const(channels)) {
    DataModel::Dataset dataset;
    dataset.fft                  = true;
    dataset.plt                  = !multipleChannels;
    dataset.groupId              = 0;
    dataset.datasetId            = index - 1;
    dataset.uniqueId             = dataset_unique_id(0, 0, index - 1);
    dataset.index                = index;
    dataset.value                = channel;
    dataset.pltMax               = maxValue;
    dataset.pltMin               = minValue;
    dataset.fftMax               = maxValue;
    dataset.fftMin               = minValue;
    dataset.fftSamples           = fftSamples;
    dataset.fftSamplingRate      = sampleRate;
    dataset.fftLogX              = true;
    dataset.fftBallistics        = true;
    dataset.fftBallisticsRelease = 100;

    if (m_hasHeader && index > 0 && index - 1 < static_cast<int>(m_channelNames.size()))
      dataset.title = m_channelNames[index - 1];
    else
      dataset.title = tr("Channel %1").arg(index);

    dataset.numericValue = SerialStudio::toDouble(dataset.value, &dataset.isNumeric);
    datasets.push_back(dataset);
    ++index;
  }

  DataModel::Group group;
  group.groupId  = 0;
  group.uniqueId = runtime_group_unique_id(0);
  group.datasets = datasets;
  group.title    = tr("Audio Input");
  if (multipleChannels)
    group.widget = QStringLiteral("multiplot");

  clear_frame(m_frame);
  m_frame.title = tr("Quick Plot");
  m_frame.sources.push_back(makeSource());
  m_frame.groups.push_back(group);
  finalize_frame(m_frame);
#else
  Q_UNUSED(channels);
#endif
}
