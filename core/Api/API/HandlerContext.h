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

namespace CSV {
class Export;
class Player;
}  // namespace CSV

namespace DataModel {
class IDashboardFrames;
}  // namespace DataModel

namespace IO {
class ConnectionManager;
}  // namespace IO

namespace MDF4 {
class Export;
class Player;
}  // namespace MDF4

#ifdef BUILD_COMMERCIAL
namespace InfluxDB {
class Export;
}  // namespace InfluxDB

namespace MQTT {
class Publisher;
}  // namespace MQTT

namespace Sessions {
class DatabaseManager;
class Export;
class Player;
}  // namespace Sessions
#endif

namespace API {

class CommandRegistry;
class Server;

/**
 * @brief The Devices, Storage, Api and dashboard-frame modules the API handlers and the Ui read
 *        through one root-bound reference set (spec 0077 T71/T72). Core services come from
 *        Core::services() and the Pipeline modules from DataModel::pipelineModules(); this set
 *        holds what only the two top libraries may name. Bound after the dashboard is adopted.
 */
struct HandlerContext {
  IO::ConnectionManager& connectionManager;
  DataModel::IDashboardFrames& dashboard;
  CommandRegistry& registry;
  Server& server;
  CSV::Player& csvPlayer;
  CSV::Export& csvExport;
  MDF4::Player& mdf4Player;
  MDF4::Export& mdf4Export;
#ifdef BUILD_COMMERCIAL
  Sessions::Player& sessionsPlayer;
  Sessions::Export& sessionsExport;
  Sessions::DatabaseManager& databaseManager;
  MQTT::Publisher& mqttPublisher;
  InfluxDB::Export& influxExport;
#endif
};

void bindHandlerContext(HandlerContext* context) noexcept;
[[nodiscard]] HandlerContext& handlerContext();

}  // namespace API
