/*
 * Serial Studio
 * https://serial-studio.com/
 *
 * Copyright (C) 2020–2026 Alex Spataru
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

#include <QtGlobal>

#include "Core/SerialStudio.h"
/**
 * @file session_context_stub.cpp
 * @brief Link-only stand-in for unit suites (spec 0032/0039): the SerialStudio meta object the
 *        FrameReader moc instantiates. Spec 0077 retired the SessionContext::current() stand-in
 *        that lived here, since no library TU reaches the context any more.
 */

/**
 * @brief Satisfies the qt_getEnumMetaObject() inlines that FrameReader's moc instantiates for
 *        the SerialStudio Q_ENUMs. Compiling the real moc instead would pull the entire
 *        invokable surface through qt_static_metacall and with it SerialStudio.cpp, which this
 *        tier deliberately never links. QObject's tables mean enum lookups return not-found;
 *        no test reads them.
 */
