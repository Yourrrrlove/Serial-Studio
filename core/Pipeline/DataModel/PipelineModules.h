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

class AppState;

namespace IO {
class PipelineHost;
}  // namespace IO

namespace DataModel {

class ControlScript;
class FrameBuilder;
class FrameParser;
class NotificationCenter;
class ProjectModel;

/**
 * @brief The Pipeline modules the libraries above read through one root-bound reference set
 *        (spec 0077 T71/T72). Storage, Api and Ui code names these instead of a module's
 *        accessor; the composition root binds the set right after the frame parser is adopted,
 *        before any module of a higher library is constructed.
 */
struct PipelineModules {
  AppState& appState;
  ProjectModel& projectModel;
  FrameBuilder& frameBuilder;
  FrameParser& frameParser;
  ControlScript& controlScript;
  IO::PipelineHost& pipelineHost;
  NotificationCenter& notifications;
};

void bindPipelineModules(PipelineModules* modules) noexcept;
[[nodiscard]] PipelineModules& pipelineModules();

}  // namespace DataModel
