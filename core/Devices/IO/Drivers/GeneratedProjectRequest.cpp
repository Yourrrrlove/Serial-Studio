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

#include "IO/Drivers/GeneratedProjectRequest.h"

#include <utility>

#include "Core/Bus/MessageBus.h"
#include "Core/Bus/Messages.h"
#include "Core/SSAssert.h"

/**
 * @brief Binds the driver that owns the request; it is the subscription's receiver, so the reply
 *        hop dies with the driver.
 */
IO::Drivers::GeneratedProjectRequest::GeneratedProjectRequest(QObject* owner)
  : m_owner(owner), m_pendingId(0), m_replied(false), m_loaded(false), m_accepted(false)
{
  SS_ASSERT_LOG(owner != nullptr);
}

/**
 * @brief Loads @p project into the editor without a save dialog (the API and CLI path) and returns
 *        whether the model took it. The model answers inside the publish, so a request nobody
 *        serves (a root without the model's wiring) reads as a refused load, never as a hang.
 */
bool IO::Drivers::GeneratedProjectRequest::load(Core::Bus::MessageBus* bus,
                                                const QJsonDocument& project)
{
  SS_ASSERT(bus != nullptr, return false);
  SS_ASSERT(!project.isEmpty(), return false);

  ensureSubscribed(*bus);
  m_pendingId  = Core::Bus::allocateRequestId();
  m_replied    = false;
  m_onFinished = nullptr;
  bus->publish<Core::Bus::LoadGeneratedProjectRequested>(project, false, m_pendingId);

  SS_ASSERT_LOG(m_replied);
  return m_replied && m_loaded;
}

/**
 * @brief Loads @p project and opens the save-as dialog (the GUI generator path); @p onFinished
 *        runs once with the load verdict and, when it loaded, whether the user accepted the save.
 */
void IO::Drivers::GeneratedProjectRequest::loadAndSave(Core::Bus::MessageBus* bus,
                                                       const QJsonDocument& project,
                                                       Finished onFinished)
{
  SS_ASSERT(bus != nullptr, return);
  SS_ASSERT(!project.isEmpty(), return);

  ensureSubscribed(*bus);
  m_pendingId  = Core::Bus::allocateRequestId();
  m_replied    = false;
  m_onFinished = std::move(onFinished);
  bus->publish<Core::Bus::LoadGeneratedProjectRequested>(project, true, m_pendingId);
}

/**
 * @brief Subscribes to the reply topic once, directly: the model serves on this thread, so the
 *        synchronous load() verdict is read right after the publish returns.
 */
void IO::Drivers::GeneratedProjectRequest::ensureSubscribed(Core::Bus::MessageBus& bus)
{
  if (m_reply.isActive())
    return;

  m_reply = bus.subscribe<Core::Bus::GeneratedProjectLoadFinished>(
    m_owner,
    [this](const std::shared_ptr<const Core::Bus::GeneratedProjectLoadFinished>& reply) {
      onReply(*reply);
    },
    Qt::DirectConnection);
}

/**
 * @brief Records the verdict for the request in flight and runs the dialog-path callback once.
 */
void IO::Drivers::GeneratedProjectRequest::onReply(
  const Core::Bus::GeneratedProjectLoadFinished& reply)
{
  if (reply.requestId != m_pendingId)
    return;

  m_replied  = true;
  m_loaded   = reply.loaded;
  m_accepted = reply.accepted;

  if (!m_onFinished)
    return;

  auto finished = std::move(m_onFinished);
  m_onFinished  = nullptr;
  finished(reply.loaded, reply.accepted);
}
