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

#include "Core/Bus/Subscription.h"

namespace Core::Bus {
struct ReplayPlayerStateChanged;
}  // namespace Core::Bus

namespace UI {

class Dashboard;

/**
 * @brief The dashboard's seam to the message bus (spec 0077), beside the facade because
 *        Dashboard.cpp is a hotpath TU that must never see the bus: the widget-structure generation
 *        and the session view state go out as retained facts, the players' view-state requests and
 *        the licence fact come in, delivered directly so a request lands before the caller's next.
 */
class DashboardWiring {
public:
  explicit DashboardWiring(Dashboard& owner);
  DashboardWiring(DashboardWiring&&)                 = delete;
  DashboardWiring(const DashboardWiring&)            = delete;
  DashboardWiring& operator=(DashboardWiring&&)      = delete;
  DashboardWiring& operator=(const DashboardWiring&) = delete;

  void wire();

private:
  void applyReplayState(const Core::Bus::ReplayPlayerStateChanged& state);

private:
  Dashboard& m_owner;
  int m_structureGeneration;

  Core::Bus::Subscription m_license;
  Core::Bus::Subscription m_replayState;
  Core::Bus::Subscription m_mirrorAttached;
  Core::Bus::Subscription m_viewStateClear;
  Core::Bus::Subscription m_viewStateRestore;
};

}  // namespace UI
