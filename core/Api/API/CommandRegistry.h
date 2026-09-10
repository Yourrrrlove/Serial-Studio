/*
 * Serial Studio
 * https://serial-studio.com/
 *
 * Copyright (C) 2020–2025 Alex Spataru
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
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QVector>

#include "API/ICheckpointStore.h"
#include "API/PathPolicy.h"
#include "Core/Api/CommandProtocol.h"

namespace API {
/**
 * @brief Signature implemented by every API command handler.
 */
using CommandFunction =
  std::function<CommandResponse(const QString& id, const QJsonObject& params)>;

/**
 * @brief Describes a registered command.
 */
struct CommandDefinition {
  QString name;
  QString description;
  QJsonObject inputSchema;
  QVector<PathParamPolicy> pathParams;
  CommandFunction handler;
};

/**
 * @brief Central registry for all available API commands. The pre-mutation checkpoint goes
 *        through the bound ICheckpointStore (spec 0077 T63); every root binds one before a
 *        command can run, so an unbound store is a logged assert and no snapshot.
 */
class CommandRegistry {
private:
  CommandRegistry();
  CommandRegistry(CommandRegistry&&)                 = delete;
  CommandRegistry(const CommandRegistry&)            = delete;
  CommandRegistry& operator=(CommandRegistry&&)      = delete;
  CommandRegistry& operator=(const CommandRegistry&) = delete;

public:
  [[nodiscard]] static CommandRegistry& instance();

  void bindCheckpointStore(ICheckpointStore& store) noexcept;
  [[nodiscard]] ICheckpointStore* checkpointStore() const noexcept;

  void registerCommand(const QString& name, const QString& description, CommandFunction handler);
  void registerCommand(const QString& name,
                       const QString& description,
                       const QJsonObject& inputSchema,
                       CommandFunction handler);

  [[nodiscard]] bool hasCommand(const QString& name) const;
  [[nodiscard]] CommandResponse execute(const QString& name,
                                        const QString& id,
                                        const QJsonObject& params);

  [[nodiscard]] QStringList availableCommands() const;
  [[nodiscard]] const QMap<QString, CommandDefinition>& commands() const;

private:
  CommandResponse buildUnknownCommandResponse(const QString& name, const QString& id) const;
  void attachErrorMetadata(const QString& name, CommandResponse& response) const;
  static QString classifyErrorCategory(const QString& commandName, const CommandResponse& response);
  static QString dryRunHintForScriptCommand(const QString& commandName);

  ICheckpointStore* m_checkpoints;
  QMap<QString, CommandDefinition> m_commands;
};

}  // namespace API
