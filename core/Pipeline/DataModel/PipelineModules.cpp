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

#include "DataModel/PipelineModules.h"

#include "Core/SSAssert.h"

static DataModel::PipelineModules* s_modules = nullptr;

/**
 * @brief Binds (or, with nullptr, unbinds) the root-owned Pipeline module set; the composition
 *        root calls this once the seven modules are adopted and again with nullptr at shutdown.
 */
void DataModel::bindPipelineModules(PipelineModules* modules) noexcept
{
  SS_ASSERT_LOG(modules == nullptr || s_modules == nullptr);
  s_modules = modules;
}

/**
 * @brief Returns the bound module set; a reach before the root bound it is a composition defect
 *        and a named fatal, the same contract the modules' own accessors keep.
 */
DataModel::PipelineModules& DataModel::pipelineModules()
{
  SS_ASSERT(s_modules != nullptr,
            qFatal("DataModel::pipelineModules() reached before the root bound it"));
  return *s_modules;
}
