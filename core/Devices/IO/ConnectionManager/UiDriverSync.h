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

#include <functional>
#include <memory>

#include "Core/Bus/Subscription.h"
#include "Core/SerialStudio.h"

class QObject;

namespace Core::Bus {
class MessageBus;
struct ProjectStructureSnapshot;
}  // namespace Core::Bus

namespace IO {

class HAL_Driver;
class DriverUiRegistry;

/**
 * @brief The single-source project settings mirror: source[0] onto the UI-config driver, the
 *        UI-config driver onto the live one, and the UI-config driver back into source[0]. All
 *        three share ONE re-entrancy latch (split, a project load echoes straight back). The
 * project is read from the retained snapshot and written over the bus: no project class is named.
 */
class UiDriverSync {
public:
  using BusTypeApplier   = std::function<void(SerialStudio::BusType)>;
  using LiveDriverLookup = std::function<HAL_Driver*(int)>;
  using ProjectSnapshot  = std::shared_ptr<const Core::Bus::ProjectStructureSnapshot>;

  UiDriverSync(DriverUiRegistry& uiDrivers,
               const SerialStudio::OperationMode& operationMode,
               const ProjectSnapshot& project,
               Core::Bus::MessageBus& bus);
  UiDriverSync(UiDriverSync&&)                 = delete;
  UiDriverSync(const UiDriverSync&)            = delete;
  UiDriverSync& operator=(UiDriverSync&&)      = delete;
  UiDriverSync& operator=(const UiDriverSync&) = delete;

  void wire(QObject& receiver, LiveDriverLookup liveDriver);
  void syncToLive(HAL_Driver* uiDriver, HAL_Driver* liveDriver) const;
  void publishSource0BusType(SerialStudio::BusType busType) const;
  void publishActiveUiDriverSettings(SerialStudio::BusType busType,
                                     HAL_Driver* uiDriver,
                                     const QObject* sender = nullptr) const;

  [[nodiscard]] HAL_Driver* driverForEditing(int deviceId);
  [[nodiscard]] bool syncFromSource0(SerialStudio::BusType current, const BusTypeApplier& applier);
  [[nodiscard]] bool captureToSource0(SerialStudio::BusType busType,
                                      HAL_Driver* uiDriver,
                                      const QObject* sender,
                                      bool autosave) const;

private:
  void serveCapture(int sourceId, int busType) const;
  void serveRestore(int sourceId);

private:
  bool m_syncingFromProject;
  LiveDriverLookup m_liveDriver;
  DriverUiRegistry& m_uiDrivers;
  const SerialStudio::OperationMode& m_operationMode;
  const ProjectSnapshot& m_project;
  Core::Bus::MessageBus& m_bus;

  Core::Bus::Subscription m_captureRequests;
  Core::Bus::Subscription m_restoreRequests;
};

}  // namespace IO
