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

#include <functional>
#include <QFlags>
#include <QMap>
#include <QString>

/**
 * @file IUserPrompter.h
 * @brief The user-prompt seam every library below Ui speaks through (spec 0077): a message box
 *        that may return the chosen button, a directory picker and a file reveal. The enum
 *        values mirror QMessageBox's so the Ui implementation converts by static_cast and a
 *        caller's `Yes | No` reads exactly as it did.
 */

namespace Core::Prompt {
/**
 * @brief Message box icon, numerically identical to QMessageBox::Icon.
 */
enum Icon : int {
  NoIcon      = 0,
  Information = 1,
  Warning     = 2,
  Critical    = 3,
  Question    = 4,
};

/**
 * @brief Message box button, numerically identical to QMessageBox::StandardButton.
 */
enum Button : int {
  NoButton        = 0x00000000,
  Ok              = 0x00000400,
  Save            = 0x00000800,
  SaveAll         = 0x00001000,
  Open            = 0x00002000,
  Yes             = 0x00004000,
  YesToAll        = 0x00008000,
  No              = 0x00010000,
  NoToAll         = 0x00020000,
  Abort           = 0x00040000,
  Retry           = 0x00080000,
  Ignore          = 0x00100000,
  Close           = 0x00200000,
  Cancel          = 0x00400000,
  Discard         = 0x00800000,
  Help            = 0x01000000,
  Apply           = 0x02000000,
  Reset           = 0x04000000,
  RestoreDefaults = 0x08000000,
};
Q_DECLARE_FLAGS(Buttons, Button)
Q_DECLARE_OPERATORS_FOR_FLAGS(Buttons)

/**
 * @brief Per-button caption overrides ("Fix Automatically" on Yes); an absent button keeps
 *        the platform text.
 */
using ButtonLabels = QMap<Button, QString>;

/**
 * @brief What the composition root binds: the Ui layer's dialogs. A prompter may be asked from
 *        any thread; the implementation marshals to the GUI thread and answers NoButton when it
 *        cannot wait for the user. The directory picker is asynchronous: the handler runs on the
 *        GUI thread once the user picked, and never runs when the dialog is dismissed.
 */
class IUserPrompter {
public:
  IUserPrompter()          = default;
  virtual ~IUserPrompter() = default;

  IUserPrompter(IUserPrompter&&)                 = delete;
  IUserPrompter(const IUserPrompter&)            = delete;
  IUserPrompter& operator=(IUserPrompter&&)      = delete;
  IUserPrompter& operator=(const IUserPrompter&) = delete;

  using DirectoryHandler = std::function<void(const QString&)>;

  [[nodiscard]] virtual int promptMessage(const QString& text,
                                          const QString& informativeText,
                                          Icon icon,
                                          const QString& windowTitle,
                                          Buttons buttons,
                                          Button defaultButton,
                                          const ButtonLabels& buttonLabels) = 0;

  virtual void promptDirectory(const QString& title,
                               const QString& initialPath,
                               DirectoryHandler onSelected) = 0;
  virtual void revealInFileManager(const QString& path)     = 0;
};
}  // namespace Core::Prompt
