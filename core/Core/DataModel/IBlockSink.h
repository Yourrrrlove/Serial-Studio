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

#include <QObject>

#include "Core/DataModel/DataBlock.h"

namespace DataModel {

/**
 * @brief One consumer of finished data blocks (spec 0077): the recorders, the streaming servers and
 *        the audio export take the one trimmed copy the publisher makes per block. ingestBlock()
 *        runs on the pipeline thread at block rate and must only enqueue; sinkActive() feeds the
 *        cached any-async-sink flag and sinkActivityChanged() must announce every edge of it.
 */
class IBlockSink : public QObject {
  Q_OBJECT

signals:
  void sinkActivityChanged();

public:
  explicit IBlockSink(QObject* parent = nullptr);
  ~IBlockSink() override;

  virtual void ingestBlock(const DataBlockPtr& block)    = 0;
  [[nodiscard]] virtual bool sinkActive() const noexcept = 0;
};

}  // namespace DataModel
