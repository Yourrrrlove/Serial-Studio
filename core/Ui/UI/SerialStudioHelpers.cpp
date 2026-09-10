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

#include "UI/SerialStudioHelpers.h"

#include "Misc/ThemeManager.h"
#include "Replay/PlayerState.h"

//--------------------------------------------------------------------------------------------------
// Construction and Core forwarders
//--------------------------------------------------------------------------------------------------

/**
 * @brief Constructs the helper object the QML engine owns.
 */
UI::SerialStudioHelpers::SerialStudioHelpers(QObject* parent) : QObject(parent) {}

/**
 * @brief Forwards the shared search predicate to QML.
 */
bool UI::SerialStudioHelpers::searchMatches(const QString& query, const QString& text)
{
  return SerialStudio::searchMatches(query, text);
}

/**
 * @brief Forwards the dashboard-tool predicate to QML.
 */
bool UI::SerialStudioHelpers::isDashboardTool(const SerialStudio::DashboardWidget w)
{
  return SerialStudio::isDashboardTool(w);
}

/**
 * @brief Forwards the painted-title predicate to QML.
 */
bool UI::SerialStudioHelpers::dashboardWidgetPaintsTitle(const SerialStudio::DashboardWidget w)
{
  return SerialStudio::dashboardWidgetPaintsTitle(w);
}

/**
 * @brief Forwards the widget icon lookup to QML.
 */
QString UI::SerialStudioHelpers::dashboardWidgetIcon(const SerialStudio::DashboardWidget w,
                                                     const bool large)
{
  return SerialStudio::dashboardWidgetIcon(w, large);
}

//--------------------------------------------------------------------------------------------------
// Replay state
//--------------------------------------------------------------------------------------------------

/**
 * @brief Checks if any playback player is currently open.
 */
bool UI::SerialStudioHelpers::isAnyPlayerOpen()
{
  return SerialStudio::isAnyPlayerOpen();
}

/**
 * @brief Returns true when a player that replays stored final values is open.
 */
bool UI::SerialStudioHelpers::isFinalValuePlayerOpen()
{
  return SerialStudio::isFinalValuePlayerOpen();
}

//--------------------------------------------------------------------------------------------------
// Theme colours
//--------------------------------------------------------------------------------------------------

/**
 * @brief Retrieves the appropriate color for a dataset based on its index.
 */
QColor UI::SerialStudioHelpers::getDatasetColor(const int index)
{
  static const auto* theme = &Misc::ThemeManager::instance();
  const auto idx           = index - 1;
  const auto colors        = theme->widgetColors();

  if (colors.isEmpty())
    return QColor(Qt::gray);

  if (idx < 0)
    return QColor(Qt::gray);

  const auto count = colors.count();
  if (idx < count)
    return colors.at(idx);

  const auto cycle    = idx / count;
  const auto position = idx % count;
  const auto offset   = (cycle * 7) % count;
  const auto colorIdx = (position + offset) % count;
  return colors.at(colorIdx);
}

/**
 * @brief Resolves a dataset's display color: valid explicit override wins, else the
 *        automatic theme-palette color for the dataset's index.
 */
QColor UI::SerialStudioHelpers::getDatasetColor(const DataModel::Dataset& dataset)
{
  if (!dataset.color.isEmpty()) {
    const auto color = QColor::fromString(dataset.color);
    if (color.isValid())
      return color;
  }

  return getDatasetColor(dataset.index);
}

/**
 * @brief Returns the shared single-accent color for non-plot widgets: the theme's first widget
 *        color, so severity stays the only varying color axis on instruments (spec 0052).
 */
QColor UI::SerialStudioHelpers::getDatasetAccentColor()
{
  return getDatasetColor(1);
}

/**
 * @brief Resolves a dataset's single-accent display color: a valid explicit override wins, else
 *        the shared accent.
 */
QColor UI::SerialStudioHelpers::getDatasetAccentColor(const DataModel::Dataset& dataset)
{
  if (!dataset.color.isEmpty()) {
    const auto color = QColor::fromString(dataset.color);
    if (color.isValid())
      return color;
  }

  return getDatasetAccentColor();
}

/**
 * @brief Returns the first valid per-dataset color override in @p group; invalid when none.
 */
QColor UI::SerialStudioHelpers::getGroupColorOverride(const DataModel::Group& group)
{
  for (const auto& dataset : group.datasets) {
    if (dataset.color.isEmpty())
      continue;

    const auto color = QColor::fromString(dataset.color);
    if (color.isValid())
      return color;
  }

  return {};
}

/**
 * @brief Returns the top gradient color for the given device source index.
 */
QColor UI::SerialStudioHelpers::getDeviceTopColor(const int sourceId)
{
  if (sourceId <= 0)
    return QColor(Qt::transparent);

  static const auto* theme = &Misc::ThemeManager::instance();
  const auto& colors       = theme->deviceColors();

  if (colors.isEmpty())
    return QColor(Qt::transparent);

  return colors.at((sourceId - 1) % colors.count()).first;
}

/**
 * @brief Returns the bottom gradient color for the given device source index.
 */
QColor UI::SerialStudioHelpers::getDeviceBottomColor(const int sourceId)
{
  if (sourceId <= 0)
    return QColor(Qt::transparent);

  static const auto* theme = &Misc::ThemeManager::instance();
  const auto& colors       = theme->deviceColors();

  if (colors.isEmpty())
    return QColor(Qt::transparent);

  return colors.at((sourceId - 1) % colors.count()).second;
}

/**
 * @brief Returns a saturated accent color for a given device index.
 */
QColor UI::SerialStudioHelpers::getDeviceColor(const int sourceId)
{
  if (sourceId <= 0)
    return QColor(Qt::transparent);

  static const auto* theme = &Misc::ThemeManager::instance();
  const auto& colors       = theme->deviceColors();
  if (colors.isEmpty())
    return QColor(Qt::transparent);

  const auto& base = colors.at((sourceId - 1) % colors.count()).first;
  const auto bg    = theme->getColor(QStringLiteral("base"));
  const bool dark  = bg.isValid() && bg.lightnessF() < 0.5;

  float h, s, l, a;
  base.getHslF(&h, &s, &l, &a);
  s = qBound(0.45f, s * 2.5f, 0.85f);
  l = dark ? 0.65f : 0.38f;
  return QColor::fromHslF(h, s, l, 1.0f);
}
