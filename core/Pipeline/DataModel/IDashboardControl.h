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

namespace DataModel {

/**
 * @brief The dashboard verbs the script APIs drive synchronously on the GUI thread (spec 0077
 *        T64): the plot horizon and clear, the four visibility toggles, and the action dispatch
 *        actionFire() ends in. The dashboard implements it and the composition root binds it.
 */
class IDashboardControl {
public:
  IDashboardControl()                                    = default;
  IDashboardControl(IDashboardControl&&)                 = delete;
  IDashboardControl(const IDashboardControl&)            = delete;
  IDashboardControl& operator=(IDashboardControl&&)      = delete;
  IDashboardControl& operator=(const IDashboardControl&) = delete;
  virtual ~IDashboardControl()                           = default;

  virtual void clearPlotData()                            = 0;
  virtual void setPoints(int points)                      = 0;
  virtual void setClockEnabled(bool enabled)              = 0;
  virtual void setTerminalEnabled(bool enabled)           = 0;
  virtual void setStopwatchEnabled(bool enabled)          = 0;
  virtual void setNotificationLogEnabled(bool enabled)    = 0;
  virtual void activateAction(int index, bool guiTrigger) = 0;

  [[nodiscard]] virtual int actionIndexForId(int actionId) const noexcept = 0;
};

}  // namespace DataModel
