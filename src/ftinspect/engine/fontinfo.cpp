// fontinfo.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "fontinfo.hpp"

#include "engine.hpp"

#include <memory>
#include <utility>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#else
#include <QStringConverter>
#include <QByteArrayView>
#endif
#include <freetype/ftmodapi.h>
#include <freetype/ttnameid.h>


void
SFNTName::get(Engine* engine,
              std::vector<SFNTName>& list)
{
  auto face = engine->currentFallbackFtFace();
  if (!face || !FT_IS_SFNT(face))
  {
    list.clear();
    return;
  }
  
  auto newSize = FT_Get_Sfnt_Name_Count(face);
  if (list.size() != static_cast<size_t>(newSize))
    list.resize(newSize);

  FT_SfntName sfntName;
  FT_SfntLangTag langTag;
  for (unsigned int i = 0; i < newSize; ++i)
  {
    FT_Get_Sfnt_Name(face, i, &sfntName);
    auto& obj = list[i];
    obj.platformID = sfntName.platform_id;
    obj.encodingID = sfntName.encoding_id;
    obj.languageID = sfntName.language_id;
    obj.nameID = sfntName.name_id;

    auto len = sfntName.string_len >= INT_MAX
                 ? INT_MAX - 1
                 : sfntName.string_len;
    obj.strBuf = QByteArray(reinterpret_cast<const char*>(sfntName.string), 
                            len);
    obj.str = sfntNameToQString(sfntName, &obj.strValid);

    if (obj.languageID >= 0x8000)
    {
      auto err = FT_Get_Sfnt_LangTag(face, obj.languageID, &langTag);
      if (!err)
        obj.langTag = utf16BEToQString(reinterpret_cast<char*>(langTag.string),
                                       langTag.string_len);
    }
  }
}


QString
SFNTName::sfntNameToQString(FT_SfntName const& sfntName, 
                            bool* outSuccess)
{
  return sfntNameToQString(sfntName.platform_id, sfntName.encoding_id,
                           reinterpret_cast<char const*>(sfntName.string),
                           sfntName.string_len,
                           outSuccess);
}


QString
SFNTName::sfntNameToQString(SFNTName const& sfntName, bool* outSuccess)
{
  return sfntNameToQString(sfntName.platformID, sfntName.encodingID,
                           sfntName.strBuf.data(), sfntName.strBuf.size(),
                           outSuccess);
}


QString
SFNTName::sfntNameToQString(unsigned short platformID,
                            unsigned short encodingID,
                            char const* str,
                            size_t size,
                            bool* outSuccess)
{
  // TODO not complete.
  if (size >= INT_MAX - 1)
    return "";

  if (outSuccess)
    *outSuccess = true;

  switch (platformID)
  {
  case TT_PLATFORM_APPLE_UNICODE:
    // All UTF-16BE.
    return utf16BEToQString(str, size);
  case TT_PLATFORM_MACINTOSH:
    if (encodingID == TT_MAC_ID_ROMAN)
      return QString::fromLatin1(str, static_cast<int>(size));

    if (outSuccess)
      *outSuccess = false;
    return "<encoding unsupported>";
  case TT_PLATFORM_ISO:
    switch (encodingID)
    {
    case TT_ISO_ID_7BIT_ASCII:
    case TT_ISO_ID_8859_1:
      return QString::fromLatin1(str, static_cast<int>(size));
    case TT_ISO_ID_10646:
      return utf16BEToQString(str, size);
    default:
      if (outSuccess)
        *outSuccess = false;
      return "<encoding unsupported>";
    }
  case TT_PLATFORM_MICROSOFT:
    switch (encodingID)
    {
      /* TT_MS_ID_SYMBOL_CS is Unicode, similar to PID/EID=3/1 */
    case TT_MS_ID_SYMBOL_CS:
    case TT_MS_ID_UNICODE_CS:
    case TT_MS_ID_UCS_4: // This is UTF-16LE as well, according to MS doc
      return utf16BEToQString(str, size);

    default:
      if (outSuccess)
        *outSuccess = false;
      return "<encoding unsupported>";
    }
  }

  if (outSuccess)
    *outSuccess = false;
  return "<platform unsupported>";
}


#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QTextCodec* utf16BECodec = QTextCodec::codecForName("UTF-16BE");
#else
QStringDecoder utf16BECvt = (QStringDecoder(QStringDecoder::Utf16BE))(size);
#endif

QString
SFNTName::utf16BEToQString(char const* str,
                           size_t size)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  if (size >= INT_MAX)
    size = INT_MAX - 1;
  return utf16BECodec->toUnicode(str, static_cast<int>(size));
#else
  return utf16BECvt(QByteArrayView(reinterpret_cast<char*>(str), size));
#endif
}


// end of fontinfo.cpp
