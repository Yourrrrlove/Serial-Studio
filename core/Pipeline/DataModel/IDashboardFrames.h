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

#include "Core/DataModel/DataBlock.h"
#include "Core/DataModel/Frame.h"

namespace DataModel {

/**
 * @brief The dashboard's frame surface the API layer reads and feeds (spec 0077 T71): the last
 *        processed frame and its display facts for the mirror publisher and the MCP resources,
 *        and the two ingest verbs a mirror viewer drives. The dashboard implements it and the
 *        composition root binds it; the per-tick and structure events travel on the bus.
 */
class IDashboardFrames {
public:
  IDashboardFrames()                                   = default;
  IDashboardFrames(IDashboardFrames&&)                 = delete;
  IDashboardFrames(const IDashboardFrames&)            = delete;
  IDashboardFrames& operator=(IDashboardFrames&&)      = delete;
  IDashboardFrames& operator=(const IDashboardFrames&) = delete;
  virtual ~IDashboardFrames()                          = default;

  [[nodiscard]] virtual const Frame& rawFrame() const         = 0;
  [[nodiscard]] virtual const Frame& processedFrame() const   = 0;
  [[nodiscard]] virtual double plotTimeRange() const noexcept = 0;
  [[nodiscard]] virtual bool frozen() const                   = 0;
  [[nodiscard]] virtual bool streamAvailable() const          = 0;

  virtual void applyBlock(const DataBlockPtr& block)                        = 0;
  virtual void applyStructureSnapshot(const StructureSnapshotPtr& snapshot) = 0;
};

}  // namespace DataModel
