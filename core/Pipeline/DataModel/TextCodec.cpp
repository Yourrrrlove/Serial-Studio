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

#include "DataModel/TextCodec.h"

#include <optional>
#include <QStringConverter>
#include <QtCore5Compat/QTextCodec>

#include "Core/SSAssert.h"

//--------------------------------------------------------------------------------------------------
// Codec lookup
//--------------------------------------------------------------------------------------------------

/**
 * @brief Returns the QStringConverter encoding for natively-supported entries.
 */
static std::optional<QStringConverter::Encoding> nativeEncoding(SerialStudio::TextEncoding enc)
{
  switch (enc) {
    case SerialStudio::EncUtf8:
      return QStringConverter::Utf8;
    case SerialStudio::EncUtf16LE:
      return QStringConverter::Utf16LE;
    case SerialStudio::EncUtf16BE:
      return QStringConverter::Utf16BE;
    case SerialStudio::EncLatin1:
      return QStringConverter::Latin1;
    case SerialStudio::EncSystem:
      return QStringConverter::System;
    default:
      return std::nullopt;
  }
}

/**
 * @brief Returns the `QTextCodec` for multi-byte East-Asian encodings.
 */
static QTextCodec* legacyCodec(SerialStudio::TextEncoding enc)
{
  const char* name = nullptr;
  switch (enc) {
    case SerialStudio::EncGbk:
      name = "GBK";
      break;
    case SerialStudio::EncGb18030:
      name = "GB18030";
      break;
    case SerialStudio::EncBig5:
      name = "Big5";
      break;
    case SerialStudio::EncShiftJis:
      name = "Shift_JIS";
      break;
    case SerialStudio::EncEucJp:
      name = "EUC-JP";
      break;
    case SerialStudio::EncEucKr:
      name = "EUC-KR";
      break;
    default:
      break;
  }

  QTextCodec* codec = name ? QTextCodec::codecForName(name) : nullptr;
  if (!codec)
    codec = QTextCodec::codecForName("UTF-8");

  SS_ASSERT_LOG(codec != nullptr);
  return codec;
}

//--------------------------------------------------------------------------------------------------
// Conversion
//--------------------------------------------------------------------------------------------------

/**
 * @brief Encodes a QString to raw bytes using the given text encoding.
 */
QByteArray SerialStudio::encodeText(const QString& text, const TextEncoding enc)
{
  if (text.isEmpty())
    return {};

  if (const auto native = nativeEncoding(enc); native.has_value()) {
    QStringEncoder encoder(*native);
    return QByteArray(encoder.encode(text));
  }

  auto* codec = legacyCodec(enc);
  SS_ASSERT(codec != nullptr, return {});
  return codec->fromUnicode(text);
}

/**
 * @brief Decodes raw bytes to a QString using the given text encoding.
 */
QString SerialStudio::decodeText(const QByteArrayView bytes, const TextEncoding enc)
{
  if (bytes.isEmpty())
    return {};

  if (const auto native = nativeEncoding(enc); native.has_value()) {
    QStringDecoder decoder(*native);
    return decoder.decode(bytes);
  }

  auto* codec = legacyCodec(enc);
  SS_ASSERT(codec != nullptr, return {});
  return codec->toUnicode(bytes.constData(), static_cast<int>(bytes.size()));
}
