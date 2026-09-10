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

#include <QByteArray>
#include <QByteArrayView>
#include <QString>

#include "Core/SerialStudio.h"

/**
 * @file TextCodec.h
 * @brief QString/byte conversion for every SerialStudio::TextEncoding. Lives in Pipeline rather
 *        than Core because the East-Asian codecs need QtCore5Compat, which Core never links.
 */

namespace SerialStudio {
[[nodiscard]] QByteArray encodeText(const QString& text, TextEncoding enc);
[[nodiscard]] QString decodeText(QByteArrayView bytes, TextEncoding enc);
}  // namespace SerialStudio
