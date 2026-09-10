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

#include "API/HandlerContext.h"

#include "Core/SSAssert.h"

static API::HandlerContext* s_context = nullptr;

/**
 * @brief Binds (or, with nullptr, unbinds) the root-owned handler context; the composition root
 *        calls this once the dashboard is adopted and again with nullptr at shutdown.
 */
void API::bindHandlerContext(HandlerContext* context) noexcept
{
  SS_ASSERT_LOG(context == nullptr || s_context == nullptr);
  s_context = context;
}

/**
 * @brief Returns the bound handler context; a reach before the root bound it is a composition
 *        defect and a named fatal, the same contract the session modules' accessors keep.
 */
API::HandlerContext& API::handlerContext()
{
  SS_ASSERT(s_context != nullptr, qFatal("API::handlerContext() reached before the root bound it"));
  return *s_context;
}
