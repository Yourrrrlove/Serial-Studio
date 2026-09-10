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

#include "DataModel/ActionBytes.h"

#include "Core/SerialStudio.h"
#include "DataModel/TextCodec.h"

/**
 * @brief Encodes an Action's TX payload (text/hex with optional EOL) to the on-wire byte array.
 */
QByteArray DataModel::get_tx_bytes(const Action& action)
{
  QByteArray b;
  const auto enc = static_cast<SerialStudio::TextEncoding>(action.txEncoding);
  if (action.binaryData)
    b = SerialStudio::hexToBytes(action.txData);
  else
    b = SerialStudio::encodeText(SerialStudio::resolveEscapeSequences(action.txData), enc);

  if (!action.eolSequence.isEmpty()) {
    const auto eol = SerialStudio::resolveEscapeSequences(action.eolSequence);
    b.append(action.binaryData ? eol.toUtf8() : SerialStudio::encodeText(eol, enc));
  }

  return b;
}
