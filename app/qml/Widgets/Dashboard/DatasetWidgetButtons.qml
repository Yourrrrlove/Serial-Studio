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

import QtQuick
import QtQuick.Controls

// Pop-out buttons for the other dashboard widgets showing one dataset (shared by the row widgets)
Row {
  id: root

  required property var windowRoot
  property var widgets: []
  property int iconSize: 18
  property int buttonSize: iconSize + 6
  property real dimmedOpacity: 0.6

  readonly property int effectiveIconSize: Math.max(18, iconSize)

  spacing: 0
  visible: widgets && widgets.length > 0

  Repeater {
    model: root.widgets

    delegate: ToolButton {
      required property var modelData

      flat: true
      background: null
      icon.color: "transparent"
      icon.width: root.effectiveIconSize
      icon.height: root.effectiveIconSize
      opacity: hovered ? 1 : root.dimmedOpacity
      width: Math.max(root.buttonSize, root.effectiveIconSize + 4)
      height: Math.max(root.buttonSize, root.effectiveIconSize + 4)
      icon.source: Cpp_Misc_IconRegistry.iconById(modelData.iconId, root.effectiveIconSize)
      onClicked: {
        if (root.windowRoot && root.windowRoot.externalWidgetRequested)
          root.windowRoot.externalWidgetRequested(modelData.windowId)
      }

      ToolTip.delay: 600
      ToolTip.visible: hovered
      ToolTip.text: qsTr("Open %1 in a separate window").arg(modelData.title)
    }
  }
}
