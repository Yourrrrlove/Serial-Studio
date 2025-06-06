/*
 * Serial Studio - https://serial-studio.github.io/
 *
 * Copyright (C) 2020-2025 Alex Spataru <https://aspatru.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "UI/Dashboard.h"
#include "Misc/ThemeManager.h"
#include "UI/Widgets/DataGrid.h"

/**
 * @brief Constructs a DataGrid widget.
 * @param index The index of the data grid in the Dashboard.
 * @param parent The parent QQuickItem (optional).
 */
Widgets::DataGrid::DataGrid(const int index, QQuickItem *parent)
  : QQuickItem(parent)
  , m_index(index)
{
  if (VALIDATE_WIDGET(SerialStudio::DashboardDataGrid, m_index))
  {
    const auto &group = GET_GROUP(SerialStudio::DashboardDataGrid, m_index);

    m_units.resize(group.datasetCount());
    m_titles.resize(group.datasetCount());
    m_values.resize(group.datasetCount());
    m_alarms.resize(group.datasetCount());

    for (int i = 0; i < group.datasetCount(); ++i)
    {
      auto dataset = group.getDataset(i);

      m_values[i] = "";
      m_alarms[i] = false;
      m_titles[i] = dataset.title();
      m_units[i] = dataset.units().isEmpty()
                       ? ""
                       : QString("[%1]").arg(dataset.units());
    }

    connect(&UI::Dashboard::instance(), &UI::Dashboard::updated, this,
            &DataGrid::updateData);

    onThemeChanged();
    connect(&Misc::ThemeManager::instance(), &Misc::ThemeManager::themeChanged,
            this, &Widgets::DataGrid::onThemeChanged);
  }
}

/**
 * @brief Returns the number of datasets in the panel.
 * @return An integer number with the number/count of datasets in the panel.
 */
int Widgets::DataGrid::count() const
{
  return m_titles.count();
}

/**
 * @brief Returns the alarm states of the datasets in the panel.
 * @return A vector of booleans representing the alarm states of the datasets.
 */
const QList<bool> &Widgets::DataGrid::alarms() const
{
  return m_alarms;
}

/**
 * @brief Returns the units of the datasets in the data grid.
 * @return A vector of strings representing the units of the datasets.
 */
const QStringList &Widgets::DataGrid::units() const
{
  return m_units;
}

/**
 * @brief Returns the colors of the datasets in the data grid.
 * @return A vector of strings representing the colors of the datasets.
 */
const QStringList &Widgets::DataGrid::colors() const
{
  return m_colors;
}

/**
 * @brief Returns the titles of the datasets in the data grid.
 * @return A vector of strings representing the titles of the datasets.
 */
const QStringList &Widgets::DataGrid::titles() const
{
  return m_titles;
}

/**
 * @brief Returns the values of the datasets in the data grid.
 * @return A vector of strings representing the values of the datasets.
 */
const QStringList &Widgets::DataGrid::values() const
{
  return m_values;
}

/**
 * @brief Updates the data grid data from the Dashboard.
 *
 * This method retrieves the latest data for this data grid from the Dashboard
 * and updates the displayed values accordingly.
 */
void Widgets::DataGrid::updateData()
{
  if (!isEnabled())
    return;

  if (VALIDATE_WIDGET(SerialStudio::DashboardDataGrid, m_index))
  {
    // Regular expression to check if the value is a number
    static const QRegularExpression regex("^[+-]?(\\d*\\.)?\\d+$");

    // Get the datagrid group and update the value readings
    bool changed = false;
    const auto &group = GET_GROUP(SerialStudio::DashboardDataGrid, m_index);
    for (int i = 0; i < group.datasetCount(); ++i)
    {
      // Get the dataset and its values
      const auto &dataset = group.getDataset(i);
      const auto alarmValue = dataset.alarm();
      auto value = dataset.value();

      // Check if dataset is a number
      bool isNumber;
      const double n = value.toDouble(&isNumber);

      // Process dataset numerical value
      bool alarm = false;
      if (isNumber)
      {
        value = QString::number(n, 'f', UI::Dashboard::instance().precision());
        alarm = (alarmValue != 0 && n >= alarmValue);
      }

      // Update the alarm state
      if (m_alarms[i] != alarm)
      {
        changed = true;
        m_alarms[i] = alarm;
      }

      // Update value text
      if (m_values[i] != value)
      {
        changed = true;
        m_values[i] = value;
      }
    }

    // Redraw the widget
    if (changed)
      Q_EMIT updated();
  }
}

/**
 * @brief Updates the colors for each dataset in the widget based on the
 *        colorscheme defined by the application's currently loaded theme.
 */
void Widgets::DataGrid::onThemeChanged()
{
  // clang-format off
  const auto colors = Misc::ThemeManager::instance().colors()["widget_colors"].toArray();
  // clang-format on

  if (VALIDATE_WIDGET(SerialStudio::DashboardDataGrid, m_index))
  {
    const auto &group = GET_GROUP(SerialStudio::DashboardDataGrid, m_index);

    m_colors.clear();
    m_colors.resize(group.datasetCount());
    for (int i = 0; i < group.datasetCount(); ++i)
    {
      const auto &dataset = group.getDataset(i);
      const auto index = dataset.index() - 1;
      const auto color = colors.count() > index
                             ? colors.at(index).toString()
                             : colors.at(index % colors.count()).toString();
      m_colors[i] = color;
    }

    Q_EMIT themeChanged();
  }
}
