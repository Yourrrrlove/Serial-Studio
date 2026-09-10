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

#include "Core/Prompt/UserPrompt.h"

#include <QDebug>

#include "Core/SSAssert.h"

static Core::Prompt::IUserPrompter* s_prompter = nullptr;

/**
 * @brief Binds (or, with null, withdraws) the prompter; the composition root calls this inside
 *        the pinned order, before any module can prompt.
 */
void Core::Prompt::setPrompter(IUserPrompter* prompter)
{
  SS_ASSERT_LOG(prompter == nullptr || s_prompter == nullptr);
  s_prompter = prompter;
}

/**
 * @brief Shows a message box through the bound prompter and returns the chosen button.
 */
int Core::Prompt::showMessageBox(const QString& text,
                                 const QString& informativeText,
                                 const Icon icon,
                                 const QString& windowTitle,
                                 const Buttons buttons,
                                 const Button defaultButton,
                                 const ButtonLabels& buttonLabels)
{
  SS_ASSERT_LOG(!text.isEmpty());
  if (!s_prompter) {
    qWarning().noquote() << "[prompt]" << windowTitle << text << informativeText;
    return defaultButton;
  }

  return s_prompter->promptMessage(
    text, informativeText, icon, windowTitle, buttons, defaultButton, buttonLabels);
}

/**
 * @brief Asks the user for a directory through the bound prompter; @p onSelected runs with the
 *        chosen path, and not at all when the dialog is dismissed or no prompter is bound.
 */
void Core::Prompt::selectDirectory(const QString& title,
                                   const QString& initialPath,
                                   IUserPrompter::DirectoryHandler onSelected)
{
  SS_ASSERT_LOG(!title.isEmpty());
  SS_ASSERT(onSelected != nullptr, return);
  if (!s_prompter)
    return;

  s_prompter->promptDirectory(title, initialPath, std::move(onSelected));
}

/**
 * @brief Reveals @p path in the host file manager through the bound prompter.
 */
void Core::Prompt::revealFile(const QString& path)
{
  SS_ASSERT(!path.isEmpty(), return);
  if (!s_prompter)
    return;

  s_prompter->revealInFileManager(path);
}
