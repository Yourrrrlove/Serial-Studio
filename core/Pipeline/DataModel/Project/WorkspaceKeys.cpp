/*
 * Serial Studio
 * https://serial-studio.com/
 *
 * Copyright (C) 2020-2025 Alex Spataru
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

#include "DataModel/Project/WorkspaceKeys.h"

#include <algorithm>

#include "Core/DataModel/Frame.h"
#include "Core/License.h"
#include "Core/SerialStudio.h"
#include "DataModel/ProjectModel.h"
#include "DataModel/WidgetResolution.h"

/**
 * @brief Encodes (widgetType, groupId, relativeIndex) into a single 64-bit key.
 */
qint64 DataModel::WorkspaceKeys::workspaceWidgetKey(int widgetType, int groupId, int relIdx)
{
  return (static_cast<qint64>(widgetType) << 40) | (static_cast<qint64>(groupId) << 20)
       | static_cast<qint64>(relIdx);
}

namespace DataModel::WorkspaceKeys {

/**
 * @brief Builds the lookup of every widget reference the project currently exposes.
 */
QHash<qint64, ResolvedWidget> buildResolvedWidgetLookup(const DataModel::ProjectModel& pm)
{
  QHash<qint64, ResolvedWidget> lookup;
  const auto& groups = pm.groups();
  const bool pro     = Core::License::activated();
  QHash<int, int> groupRunning;
  QHash<int, int> datasetRunning;

  datasetRunning.insert(static_cast<int>(SerialStudio::DashboardExtension),
                        SerialStudio::extensionGroupWidgetCount(groups));

  for (const auto& g : groups) {
    if (!SerialStudio::groupEligibleForWorkspace(g))
      continue;

    auto groupKey = SerialStudio::getDashboardWidget(g);
    if (groupKey == SerialStudio::DashboardPlot3D && !pro)
      groupKey = SerialStudio::DashboardMultiPlot;

    const bool isEmptyOutputPanel =
      g.groupType == DataModel::GroupType::Output && g.outputWidgets.empty();

    if (SerialStudio::groupWidgetEligibleForWorkspace(groupKey) && !isEmptyOutputPanel) {
      const int typeKey = static_cast<int>(groupKey);
      const int relIdx  = groupRunning.value(typeKey, 0);
      groupRunning.insert(typeKey, relIdx + 1);

      ResolvedWidget entry;
      entry.groupTitle    = g.title;
      entry.datasetTitle  = QString();
      entry.uniqueId      = g.uniqueId;
      entry.isGroupWidget = true;
      lookup.insert(workspaceWidgetKey(typeKey, g.uniqueId, relIdx), entry);
    }

    const auto recordDatasetWidget = [&](const DataModel::Dataset& ds,
                                         SerialStudio::DashboardWidget k) {
      const int typeKey = static_cast<int>(k);
      const int relIdx  = datasetRunning.value(typeKey, 0);
      datasetRunning.insert(typeKey, relIdx + 1);

      ResolvedWidget entry;
      entry.groupTitle   = g.title;
      entry.datasetTitle = ds.title;
      entry.uniqueId     = ds.uniqueId;
      lookup.insert(workspaceWidgetKey(typeKey, g.uniqueId, relIdx), entry);
    };

    const auto walkDatasetWidgets = [&](const DataModel::Dataset& ds) {
      const auto keys = SerialStudio::getDashboardWidgets(ds);
      for (const auto& k : keys)
        if (SerialStudio::datasetWidgetEligibleForWorkspace(k))
          recordDatasetWidget(ds, k);
    };

    for (const auto& ds : g.datasets)
      walkDatasetWidgets(ds);

    const bool groupHasLed =
      std::any_of(g.datasets.begin(), g.datasets.end(), [](const DataModel::Dataset& ds) {
        return !ds.hideOnDashboard && ds.led;
      });
    if (groupHasLed) {
      const int typeKey = static_cast<int>(SerialStudio::DashboardLED);
      const int relIdx  = groupRunning.value(typeKey, 0);
      groupRunning.insert(typeKey, relIdx + 1);

      ResolvedWidget entry;
      entry.groupTitle    = g.title;
      entry.datasetTitle  = QString();
      entry.uniqueId      = g.uniqueId;
      entry.isGroupWidget = true;
      entry.isLedPanel    = true;
      lookup.insert(workspaceWidgetKey(typeKey, g.uniqueId, relIdx), entry);
    }
  }

  return lookup;
}

}  // namespace DataModel::WorkspaceKeys
