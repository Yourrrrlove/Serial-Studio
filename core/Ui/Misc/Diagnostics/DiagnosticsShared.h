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

#pragma once

#include <QList>
#include <QString>

#include "Core/DiagnosticsTypes.h"
#include "Misc/ProblemCenter.h"

namespace Misc::Diagnostics {

/**
 * @brief Maps a verdict onto the problem-center severity that renders it.
 */
[[nodiscard]] inline ProblemCenter::Severity severityOf(Verdict verdict)
{
  if (verdict == Verdict::Failure)
    return ProblemCenter::Error;

  if (verdict == Verdict::Warning)
    return ProblemCenter::Warning;

  return ProblemCenter::Info;
}

/**
 * @brief Lowers one result into the problem-center finding the panel renders; the checker id is
 *        stamped by the collector after the run.
 */
[[nodiscard]] inline ProblemCenter::Finding toFinding(const Result& result)
{
  ProblemCenter::Finding finding;
  finding.severity    = severityOf(result.verdict);
  finding.code        = result.code;
  finding.title       = result.title;
  finding.remedy      = result.remedy;
  finding.explanation = result.explanation;
  return finding;
}

}  // namespace Misc::Diagnostics
