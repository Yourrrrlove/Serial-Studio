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

#include "Replay/LinkGate.h"

#include "Core/Bus/MessageBus.h"
#include "Core/Bus/Messages.h"
#include "Core/SSAssert.h"

/**
 * @brief Whether a device link is up, read from the retained link state (spec 0077 T65): a
 *        replay player asks this before it takes over the pipeline, and a player without a bus
 *        answers "no link" rather than reaching for the connection manager.
 */
bool Replay::linkConnected(const Core::Bus::MessageBus* bus) noexcept
{
  SS_ASSERT_LOG(bus != nullptr);
  if (bus == nullptr)
    return false;

  const auto link = bus->latest<Core::Bus::ConnectionStateChanged>();
  return link && link->connected;
}

/**
 * @brief Asks the connection manager to drop every device. Delivery is direct on the GUI thread,
 *        so the link is down when this returns, exactly as the direct call it replaces was.
 */
void Replay::requestDisconnect(Core::Bus::MessageBus* bus)
{
  SS_ASSERT(bus != nullptr, return);
  bus->publish<Core::Bus::DisconnectRequested>(0);
}

/**
 * @brief The gate every replay player runs before it opens a file: no link means go ahead; a live
 *        link asks the user with @p title / @p question and drops the link on Yes. Returns false
 *        when the user keeps the link, in which case the player must not open the file.
 */
bool Replay::ensureLinkReleased(Core::Bus::MessageBus* bus,
                                const QString& title,
                                const QString& question,
                                const Core::Prompt::Icon icon,
                                const QString& windowTitle)
{
  if (!linkConnected(bus))
    return true;

  const int response = Core::Prompt::showMessageBox(
    title, question, icon, windowTitle, Core::Prompt::No | Core::Prompt::Yes);
  SS_ASSERT_LOG(response == Core::Prompt::Yes || response == Core::Prompt::No);
  if (response != Core::Prompt::Yes)
    return false;

  requestDisconnect(bus);
  return true;
}
