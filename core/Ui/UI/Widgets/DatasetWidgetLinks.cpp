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

#include "UI/Widgets/DatasetWidgetLinks.h"

#include <QVariantMap>

#include "Core/DataModel/Frame.h"
#include "Core/SerialStudio.h"
#include "UI/Dashboard.h"

/**
 * @brief One {windowId, icon (16 px path), iconId (registry id), title} map per dashboard widget
 *        displaying @p dataset, in dashboard order with plots last so the right-aligned plot
 *        button lands in one column; identity is the source/group/dataset triple, not the row.
 */
QVariantList Widgets::datasetWidgetLinks(UI::Dashboard& dashboard,
                                         const DataModel::Dataset& dataset)
{
  QVariantList widgets;
  QVariantList plots;
  const auto& map = dashboard.widgetMap();
  for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
    const auto type = it.value().first;
    if (!SerialStudio::isDatasetWidget(type))
      continue;

    const auto& shown = dashboard.getDatasetWidget(type, it.value().second);
    if (shown.sourceId != dataset.sourceId || shown.groupId != dataset.groupId
        || shown.datasetId != dataset.datasetId)
      continue;

    QVariantMap entry;
    entry.insert(QStringLiteral("windowId"), it.key());
    entry.insert(QStringLiteral("icon"), SerialStudio::dashboardWidgetIcon(type));
    entry.insert(QStringLiteral("iconId"), SerialStudio::dashboardWidgetIconId(type));
    entry.insert(QStringLiteral("title"), SerialStudio::dashboardWidgetTitle(type));

    if (type == SerialStudio::DashboardPlot)
      plots.append(entry);
    else
      widgets.append(entry);
  }

  widgets += plots;
  return widgets;
}
