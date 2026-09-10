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

namespace Core::Bus {
class MessageBus;
}  // namespace Core::Bus

namespace Misc {
class IconRegistry;
class TimerEvents;
class Translator;
class WorkspaceManager;
}  // namespace Misc

namespace Core {

/**
 * @brief The Core-owned services every library above reads through one root-bound reference set
 *        (spec 0077 T71/T72): the bus and the four Core singletons. A library never constructs
 *        one of these; the composition root builds them in the pinned order and binds the set
 *        before the first module above Core exists, so reaching it is never a construction.
 */
struct Services {
  Core::Bus::MessageBus& bus;
  Misc::Translator& translator;
  Misc::TimerEvents& timerEvents;
  Misc::WorkspaceManager& workspaceManager;
  Misc::IconRegistry& iconRegistry;
};

void bindServices(Services* services) noexcept;
[[nodiscard]] Services& services();

}  // namespace Core
