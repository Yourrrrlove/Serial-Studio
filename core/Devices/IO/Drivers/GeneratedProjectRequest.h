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
#include <QJsonDocument>
#include <QObject>

#include "Core/Bus/Subscription.h"

namespace Core::Bus {
class MessageBus;
struct GeneratedProjectLoadFinished;
}  // namespace Core::Bus

namespace IO::Drivers {

/**
 * @brief A driver's request to load the project it generated (spec 0077 T57), answered by the
 *        project model under the same request id. The model serves the request directly on the
 *        driver's thread, so load() returns the verdict synchronously (the API and CLI want a
 *        bool); loadAndSave() reports through a callback once the save dialog answers.
 */
class GeneratedProjectRequest {
public:
  using Finished = std::function<void(bool loaded, bool accepted)>;

  explicit GeneratedProjectRequest(QObject* owner);
  GeneratedProjectRequest(GeneratedProjectRequest&&)                 = delete;
  GeneratedProjectRequest(const GeneratedProjectRequest&)            = delete;
  GeneratedProjectRequest& operator=(GeneratedProjectRequest&&)      = delete;
  GeneratedProjectRequest& operator=(const GeneratedProjectRequest&) = delete;

  [[nodiscard]] bool load(Core::Bus::MessageBus* bus, const QJsonDocument& project);
  void loadAndSave(Core::Bus::MessageBus* bus, const QJsonDocument& project, Finished onFinished);

private:
  void ensureSubscribed(Core::Bus::MessageBus& bus);
  void onReply(const Core::Bus::GeneratedProjectLoadFinished& reply);

private:
  QObject* m_owner;
  quint64 m_pendingId;
  bool m_replied;
  bool m_loaded;
  bool m_accepted;
  Finished m_onFinished;
  Core::Bus::Subscription m_reply;
};

}  // namespace IO::Drivers
