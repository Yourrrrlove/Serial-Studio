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

#include <QString>

#include "Core/Prompt/IUserPrompter.h"

/**
 * @file UserPrompt.h
 * @brief The free functions a library calls to prompt the user; they forward to the prompter the
 *        composition root bound. Unbound (a headless root that never binds one), a message box
 *        logs and answers its default button, the picker never calls back and reveal does nothing.
 *        The message box's answer is deliberately discardable: most callers only inform, and the
 *        few that branch on a button read the return.
 */

namespace Core::Prompt {
void setPrompter(IUserPrompter* prompter);

int showMessageBox(const QString& text,
                   const QString& informativeText   = QString(),
                   Icon icon                        = Information,
                   const QString& windowTitle       = QString(),
                   Buttons buttons                  = Ok,
                   Button defaultButton             = NoButton,
                   const ButtonLabels& buttonLabels = ButtonLabels());
void selectDirectory(const QString& title,
                     const QString& initialPath,
                     IUserPrompter::DirectoryHandler onSelected);

void revealFile(const QString& path);
}  // namespace Core::Prompt
