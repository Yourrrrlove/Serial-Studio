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
#include <QHash>
#include <QVariant>

#include "Core/DataModel/Frame.h"

class QStandardItem;

namespace DataModel {

class ProjectEditor;
class ProjectModel;

/**
 * @brief Homogeneous multi-selection editing: detects a selection of two or more datasets or
 *        output widgets, builds the aggregate form (identity rows greyed, disagreeing rows
 *        marked Mixed) and fans one field edit out across every member as a single undo frame
 *        and one autosave. The selection membership lives on the facade.
 */
class EditorMultiSelect {
  Q_DECLARE_TR_FUNCTIONS(DataModel::ProjectEditor)

public:
  explicit EditorMultiSelect(ProjectEditor& editor, ProjectModel& model);
  EditorMultiSelect(EditorMultiSelect&&)                 = delete;
  EditorMultiSelect(const EditorMultiSelect&)            = delete;
  EditorMultiSelect& operator=(EditorMultiSelect&&)      = delete;
  EditorMultiSelect& operator=(const EditorMultiSelect&) = delete;

  [[nodiscard]] bool tryMultiSelection();

  void buildMultiDatasetModel();
  void changeDatasetOptionForSelection(int option, bool checked);

private:
  [[nodiscard]] QHash<int, QVariant> datasetEditValues(const DataModel::Dataset& dataset);
  [[nodiscard]] QHash<int, QVariant> outputWidgetEditValues(const DataModel::OutputWidget& widget);

  void buildMultiSelectionModel();
  void buildMultiOutputWidgetModel();
  void onMultiSelectionItemChanged(QStandardItem* item);
  void fanOutputWidgetSelectionEdit(QStandardItem* item);

private:
  ProjectEditor& m_editor;
  ProjectModel& m_model;
};

}  // namespace DataModel
