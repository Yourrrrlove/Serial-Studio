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

#include <QString>
#include <QVariantList>

namespace API {

/**
 * @brief The project checkpoint store the API takes before a destructive command (spec 0077
 *        T63): a snapshot is a checkpoint, never a save (doc/claude/architecture/ai.md). The
 *        backup manager implements it and the composition root binds it into the registry.
 */
class ICheckpointStore {
public:
  ICheckpointStore()                                   = default;
  ICheckpointStore(ICheckpointStore&&)                 = delete;
  ICheckpointStore(const ICheckpointStore&)            = delete;
  ICheckpointStore& operator=(ICheckpointStore&&)      = delete;
  ICheckpointStore& operator=(const ICheckpointStore&) = delete;
  virtual ~ICheckpointStore()                          = default;

  [[nodiscard]] virtual QString snapshot(const QString& label) = 0;
  [[nodiscard]] virtual bool restore(const QString& path)      = 0;
  [[nodiscard]] virtual QVariantList list(int limit) const     = 0;
  [[nodiscard]] virtual QString backupDirectory() const        = 0;
};

}  // namespace API
