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

#include <QCoreApplication>

class QStandardItem;

namespace MQTT {
class Publisher;
class PublisherScriptEditor;
}  // namespace MQTT

namespace DataModel {

class ProjectEditor;
class ProjectModel;

/**
 * @brief The MQTT Publisher form: builds its sections from the live Publisher singleton, pushes
 *        edits straight back into it, and owns the parentless script-editor dialog (a QWidget no
 *        Qt parent chain reaches, so the destructor is its only release path). Without
 *        BUILD_COMMERCIAL the form is an empty model.
 */
class EditorMqtt {
  Q_DECLARE_TR_FUNCTIONS(DataModel::ProjectEditor)

public:
  explicit EditorMqtt(ProjectEditor& editor);
  ~EditorMqtt();
  EditorMqtt(EditorMqtt&&)                 = delete;
  EditorMqtt(const EditorMqtt&)            = delete;
  EditorMqtt& operator=(EditorMqtt&&)      = delete;
  EditorMqtt& operator=(const EditorMqtt&) = delete;

  void openMqttScriptEditor();
  void buildMqttPublisherModel();

private:
  void onMqttPublisherItemChanged(QStandardItem* item);
#ifdef BUILD_COMMERCIAL
  void buildMqttSslSection(const MQTT::Publisher& pub, bool enabled);
  void buildMqttBrokerSection(const MQTT::Publisher& pub, bool enabled);
  void buildMqttSparkplugSection(const MQTT::Publisher& pub, bool enabled);
  void buildMqttPublishingSection(const MQTT::Publisher& pub, bool enabled);
  void buildMqttBrokerCredentials(const MQTT::Publisher& pub, bool enabled);
#endif

private:
  ProjectEditor& m_editor;
#ifdef BUILD_COMMERCIAL
  MQTT::Publisher& m_mqttPublisher;

  MQTT::PublisherScriptEditor* m_mqttScriptEditor;
#endif
};

}  // namespace DataModel
