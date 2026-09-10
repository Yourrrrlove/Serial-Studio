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

namespace Misc {
class Translator;
}  // namespace Misc

namespace DataModel {

class ProjectEditor;
class ProjectModel;

/**
 * @brief Connects the project model, the translator, the widget-extension catalog and the
 *        connection manager to the editor's rebuild, refresh and select-after-create bookkeeping.
 *        Every connection targets the facade QObject, so its lifetime is the editor's; the facade
 *        constructor runs the nine groups in order.
 */
class EditorWiring {
public:
  explicit EditorWiring(ProjectEditor& editor, ProjectModel& model);
  EditorWiring(EditorWiring&&)                 = delete;
  EditorWiring(const EditorWiring&)            = delete;
  EditorWiring& operator=(EditorWiring&&)      = delete;
  EditorWiring& operator=(const EditorWiring&) = delete;

  void wireGroupSignals();
  void wireSourceSignals();
  void wireActionSignals();
  void wireDatasetSignals();
  void wireExternalSignals();
  void wireEditorSelfSignals();
  void wireSelectionRequests();
  void wireOutputWidgetSignals();
  void wireProjectModelRebuilds();

private:
  void wireProjectFileSignals();

private:
  ProjectEditor& m_editor;
  ProjectModel& m_model;
  Misc::Translator& m_translator;

  QString m_lastJsonFilePath;
};

}  // namespace DataModel
