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

#include <QHash>
#include <QList>
#include <QVector>
#include <utility>

namespace DataModel {

/**
 * @brief Packs a (sourceId, uniqueId) pair into one replay hash key. Both the replay window and
 *        the ring snapshot key on it: uniqueIds repeat across sources, so a sourceId-free key
 *        collapses two plots onto one entry.
 */
[[nodiscard]] inline qint64 replaySeekKey(int sourceId, int uniqueId) noexcept
{
  return (static_cast<qint64>(sourceId) << 32) | static_cast<quint32>(uniqueId);
}

/**
 * @brief What a replay player asks of the plot surface while scrubbing (spec 0077 T64): the
 *        window it must fill, the series the layout wants, the bulk fill and the clear. The
 *        dashboard implements it and the composition root binds it into every player; the ~30 Hz
 *        scrub path keeps its bulk-copy shape, one virtual call per tick.
 */
class IReplayPlotSink {
public:
  IReplayPlotSink()                                  = default;
  IReplayPlotSink(IReplayPlotSink&&)                 = delete;
  IReplayPlotSink(const IReplayPlotSink&)            = delete;
  IReplayPlotSink& operator=(IReplayPlotSink&&)      = delete;
  IReplayPlotSink& operator=(const IReplayPlotSink&) = delete;
  virtual ~IReplayPlotSink()                         = default;

  [[nodiscard]] virtual int points() const noexcept                         = 0;
  [[nodiscard]] virtual double plotTimeRange() const noexcept               = 0;
  [[nodiscard]] virtual QList<std::pair<int, int>> replaySeekSeries() const = 0;

  virtual void clearPlotData()                                                  = 0;
  virtual void bulkLoadPlotWindow(const QVector<double>& timesSec,
                                  const QHash<qint64, QVector<double>>& series) = 0;
};

}  // namespace DataModel
