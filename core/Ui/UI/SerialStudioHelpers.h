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

#pragma once

#include <QColor>
#include <QObject>

#include "Core/DataModel/Frame.h"
#include "Core/SerialStudio.h"

namespace UI {
/**
 * @brief The QML-facing helper singleton that took over the SerialStudio class's invokables when
 *        the enums became a Core namespace (spec 0077): theme colours, icon lookups and the replay
 *        predicate, everything that needs a Ui or Storage module. Registered by the composition
 *        root as the `SerialStudioHelpers` singleton of the `SerialStudio` QML module.
 */
class SerialStudioHelpers : public QObject {
  Q_OBJECT

public:
  explicit SerialStudioHelpers(QObject* parent = nullptr);

  // clang-format off
  Q_INVOKABLE [[nodiscard]] static bool searchMatches(const QString& query, const QString& text);
  Q_INVOKABLE [[nodiscard]] static bool isDashboardTool(SerialStudio::DashboardWidget w);
  Q_INVOKABLE [[nodiscard]] static bool dashboardWidgetPaintsTitle(SerialStudio::DashboardWidget w);
  Q_INVOKABLE [[nodiscard]] static QString dashboardWidgetIcon(SerialStudio::DashboardWidget w, bool large = false);
  // clang-format on

  Q_INVOKABLE [[nodiscard]] static bool isAnyPlayerOpen();
  Q_INVOKABLE [[nodiscard]] static bool isFinalValuePlayerOpen();

  Q_INVOKABLE [[nodiscard]] static QColor getDatasetColor(int index);
  [[nodiscard]] static QColor getDatasetColor(const DataModel::Dataset& dataset);
  Q_INVOKABLE [[nodiscard]] static QColor getDatasetAccentColor();
  [[nodiscard]] static QColor getDatasetAccentColor(const DataModel::Dataset& dataset);
  [[nodiscard]] static QColor getGroupColorOverride(const DataModel::Group& group);
  Q_INVOKABLE [[nodiscard]] static QColor getDeviceColor(int sourceId);
  Q_INVOKABLE [[nodiscard]] static QColor getDeviceTopColor(int sourceId);
  Q_INVOKABLE [[nodiscard]] static QColor getDeviceBottomColor(int sourceId);
};
}  // namespace UI
