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
#include <QStringList>

#include "Core/Api/CommandProtocol.h"

namespace API {

/**
 * @brief The command surface a script or an assistant tool drives (spec 0077 T60): one
 *        synchronous execute() with the GUI-thread contract the handlers assume, plus discovery.
 *        The command handler implements it and the composition root binds it into the scripting
 *        layer, so no script API names the handler, the registry or a handler class.
 */
class ICommandExecutor {
public:
  ICommandExecutor()                                   = default;
  ICommandExecutor(ICommandExecutor&&)                 = delete;
  ICommandExecutor(const ICommandExecutor&)            = delete;
  ICommandExecutor& operator=(ICommandExecutor&&)      = delete;
  ICommandExecutor& operator=(const ICommandExecutor&) = delete;
  virtual ~ICommandExecutor()                          = default;

  [[nodiscard]] virtual CommandResponse execute(const CommandRequest& request) = 0;
  [[nodiscard]] virtual bool hasCommand(const QString& name) const             = 0;
  [[nodiscard]] virtual QStringList commandNames() const                       = 0;
};

}  // namespace API
