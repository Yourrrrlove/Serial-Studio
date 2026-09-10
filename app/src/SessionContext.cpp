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

#include "SessionContext.h"

#include "API/HandlerContext.h"
#include "AppState.h"
#include "Console/Handler.h"
#include "Core/Bus/MessageBus.h"
#include "Core/Services.h"
#include "Core/SSAssert.h"
#include "DataModel/FrameBuilder.h"
#include "DataModel/NotificationCenter.h"
#include "DataModel/PipelineModules.h"
#include "DataModel/ProjectModel.h"
#include "DataModel/Scripting/FrameParser.h"
#include "IO/ConnectionManager.h"
#include "IO/PipelineHost.h"
#include "UI/Dashboard.h"

//--------------------------------------------------------------------------------------------------
// Construction & publication
//--------------------------------------------------------------------------------------------------

/**
 * @brief Constructs a session context. The body stays empty on purpose: a context that
 *        constructs nothing can be created at any point of startup without adding a
 *        construction edge to the pinned module order, and cannot re-enter the current()
 *        Meyers guard from a module constructor.
 */
SessionContext::SessionContext(int session_id) : m_sessionId(session_id) {}

/**
 * @brief Destroys the session context. Adopted slots are released by shutdown() while qApp is
 *        alive; reaching this destructor with a slot still filled means shutdown() never ran.
 */
SessionContext::~SessionContext() {}

/**
 * @brief Returns the process-default session context. This is the only sanctioned global
 *        reach: the composition root pins it, and an injected class passes it from its own
 *        instance() accessor. Never call it from a method body.
 */
SessionContext& SessionContext::current()
{
  static SessionContext context(0);
  return context;
}

/**
 * @brief Returns the identifier of this session; 0 is the process-default context.
 */
int SessionContext::sessionId() const noexcept
{
  return m_sessionId;
}

/**
 * @brief Whether slot 0, the message bus, is adopted; the Core service bootstrap runs before the
 *        pinned order and the root must not adopt a second bus.
 */
bool SessionContext::hasBus() const noexcept
{
  return m_bus != nullptr;
}

/**
 * @brief Whether every session subsystem is owned by this context. True from the moment the
 *        composition root finishes the pinned order until shutdown() releases the slots.
 */
bool SessionContext::sealed() const noexcept
{
  return m_bus && m_appState && m_dashboard && m_console && m_frameParser && m_frameBuilder
      && m_projectModel && m_pipelineHost && m_connectionManager && m_notifications;
}

//--------------------------------------------------------------------------------------------------
// Ownership
//--------------------------------------------------------------------------------------------------

/**
 * @brief Takes ownership of this session's message bus, slot 0 of the pinned order.
 */
void SessionContext::adoptBus(std::unique_ptr<Core::Bus::MessageBus> bus)
{
  SS_ASSERT(!m_bus, return);
  SS_ASSERT(bus != nullptr, return);
  m_bus = std::move(bus);
}

/**
 * @brief Takes ownership of this session's application state.
 */
void SessionContext::adoptAppState(std::unique_ptr<AppState> module)
{
  SS_ASSERT(!m_appState, return);
  SS_ASSERT(module != nullptr, return);
  m_appState = std::move(module);
  AppState::bindInstance(m_appState.get());
}

/**
 * @brief Takes ownership of this session's dashboard model.
 */
void SessionContext::adoptDashboard(std::unique_ptr<UI::Dashboard> module)
{
  SS_ASSERT(!m_dashboard, return);
  SS_ASSERT(module != nullptr, return);
  m_dashboard = std::move(module);
  UI::Dashboard::bindInstance(m_dashboard.get());
}

/**
 * @brief Takes ownership of this session's console handler.
 */
void SessionContext::adoptConsole(std::unique_ptr<Console::Handler> module)
{
  SS_ASSERT(!m_console, return);
  SS_ASSERT(module != nullptr, return);
  m_console = std::move(module);
  Console::Handler::bindInstance(m_console.get());
}

/**
 * @brief Takes ownership of this session's frame parser.
 */
void SessionContext::adoptFrameParser(std::unique_ptr<DataModel::FrameParser> module)
{
  SS_ASSERT(!m_frameParser, return);
  SS_ASSERT(module != nullptr, return);
  m_frameParser = std::move(module);
  DataModel::FrameParser::bindInstance(m_frameParser.get());
}

/**
 * @brief Takes ownership of this session's frame builder.
 */
void SessionContext::adoptFrameBuilder(std::unique_ptr<DataModel::FrameBuilder> module)
{
  SS_ASSERT(!m_frameBuilder, return);
  SS_ASSERT(module != nullptr, return);
  m_frameBuilder = std::move(module);
  DataModel::FrameBuilder::bindInstance(m_frameBuilder.get());
}

/**
 * @brief Takes ownership of this session's project document.
 */
void SessionContext::adoptProjectModel(std::unique_ptr<DataModel::ProjectModel> module)
{
  SS_ASSERT(!m_projectModel, return);
  SS_ASSERT(module != nullptr, return);
  m_projectModel = std::move(module);
  DataModel::ProjectModel::bindInstance(m_projectModel.get());
}

/**
 * @brief Takes ownership of this session's pipeline host.
 */
void SessionContext::adoptPipelineHost(std::unique_ptr<IO::PipelineHost> module)
{
  SS_ASSERT(!m_pipelineHost, return);
  SS_ASSERT(module != nullptr, return);
  m_pipelineHost = std::move(module);
  IO::PipelineHost::bindInstance(m_pipelineHost.get());
}

/**
 * @brief Takes ownership of this session's connection manager.
 */
void SessionContext::adoptConnectionManager(std::unique_ptr<IO::ConnectionManager> module)
{
  SS_ASSERT(!m_connectionManager, return);
  SS_ASSERT(module != nullptr, return);
  m_connectionManager = std::move(module);
  IO::ConnectionManager::bindInstance(m_connectionManager.get());
}

/**
 * @brief Takes ownership of this session's notification center.
 */
void SessionContext::adoptNotifications(std::unique_ptr<DataModel::NotificationCenter> module)
{
  SS_ASSERT(!m_notifications, return);
  SS_ASSERT(module != nullptr, return);
  m_notifications = std::move(module);
  DataModel::NotificationCenter::bindInstance(m_notifications.get());
}

//--------------------------------------------------------------------------------------------------
// Teardown
//--------------------------------------------------------------------------------------------------

/**
 * @brief Releases every adopted subsystem in exact reverse pinned order, the bus last, while qApp
 *        is alive and after the QML engine died (INV-6). An abandoned processing thread is the one
 *        exception: parser, builder and host leak AND stay bound, since the thread's wirings
 *        still reach them.
 */
void SessionContext::shutdown()
{
  UI::Dashboard::bindInstance(nullptr);
  m_dashboard.reset();

  const bool pipelineAbandoned = m_pipelineHost && m_pipelineHost->pipelineAbandoned();
  if (pipelineAbandoned) {
    (void)m_frameParser.release();
    (void)m_frameBuilder.release();
    (void)m_pipelineHost.release();
  }

  if (!pipelineAbandoned) {
    DataModel::FrameParser::bindInstance(nullptr);
    IO::PipelineHost::bindInstance(nullptr);
    DataModel::FrameBuilder::bindInstance(nullptr);
  }

  m_frameParser.reset();
  Console::Handler::bindInstance(nullptr);
  m_console.reset();
  IO::ConnectionManager::bindInstance(nullptr);
  m_connectionManager.reset();
  m_pipelineHost.reset();
  m_frameBuilder.reset();
  AppState::bindInstance(nullptr);
  m_appState.reset();
  DataModel::ProjectModel::bindInstance(nullptr);
  m_projectModel.reset();
  DataModel::NotificationCenter::bindInstance(nullptr);
  m_notifications.reset();
  API::bindHandlerContext(nullptr);
  DataModel::bindPipelineModules(nullptr);
  Core::bindServices(nullptr);
  m_bus.reset();
}

//--------------------------------------------------------------------------------------------------
// Session-scoped subsystems (all nine owned; a reach before adoption is a named fatal)
//--------------------------------------------------------------------------------------------------

/**
 * @brief Returns the message bus every module of this session publishes on (spec 0077).
 */
Core::Bus::MessageBus& SessionContext::bus() const
{
  SS_ASSERT(m_bus != nullptr, qFatal("SessionContext: %s accessed before adoption", "bus"));
  return *m_bus;
}

/**
 * @brief Returns the application state (operation mode, project path, frame config).
 */
AppState& SessionContext::appState() const
{
  SS_ASSERT(m_appState != nullptr,
            qFatal("SessionContext: %s accessed before adoption", "appState"));
  return *m_appState;
}

/**
 * @brief Returns the console handler owning this session's terminal buffer.
 */
Console::Handler& SessionContext::console() const
{
  SS_ASSERT(m_console != nullptr, qFatal("SessionContext: %s accessed before adoption", "console"));
  return *m_console;
}

/**
 * @brief Returns the dashboard model driven by this session's frames.
 */
UI::Dashboard& SessionContext::dashboard() const
{
  SS_ASSERT(m_dashboard != nullptr,
            qFatal("SessionContext: %s accessed before adoption", "dashboard"));
  return *m_dashboard;
}

/**
 * @brief Returns the frame parser holding this session's per-source parse engines.
 */
DataModel::FrameParser& SessionContext::frameParser() const
{
  SS_ASSERT(m_frameParser != nullptr,
            qFatal("SessionContext: %s accessed before adoption", "frameParser"));
  return *m_frameParser;
}

/**
 * @brief Returns the frame builder at the head of this session's parse pipeline.
 */
DataModel::FrameBuilder& SessionContext::frameBuilder() const
{
  SS_ASSERT(m_frameBuilder != nullptr,
            qFatal("SessionContext: %s accessed before adoption", "frameBuilder"));
  return *m_frameBuilder;
}

/**
 * @brief Returns the project document of this session.
 */
DataModel::ProjectModel& SessionContext::projectModel() const
{
  SS_ASSERT(m_projectModel != nullptr,
            qFatal("SessionContext: %s accessed before adoption", "projectModel"));
  return *m_projectModel;
}

/**
 * @brief Returns the pipeline host owning this session's frame-processing thread.
 */
IO::PipelineHost& SessionContext::pipelineHost() const
{
  SS_ASSERT(m_pipelineHost != nullptr,
            qFatal("SessionContext: %s accessed before adoption", "pipelineHost"));
  return *m_pipelineHost;
}

/**
 * @brief Returns the connection manager owning this session's device drivers.
 */
IO::ConnectionManager& SessionContext::connectionManager() const
{
  SS_ASSERT(m_connectionManager != nullptr,
            qFatal("SessionContext: %s accessed before adoption", "connectionManager"));
  return *m_connectionManager;
}

/**
 * @brief Returns the notification center collecting this session's user-facing events.
 */
DataModel::NotificationCenter& SessionContext::notifications() const
{
  SS_ASSERT(m_notifications != nullptr,
            qFatal("SessionContext: %s accessed before adoption", "notifications"));
  return *m_notifications;
}
