// charmap.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "charmap.hpp"

#include <QHash>

#include <freetype/freetype.h>
#include <freetype/tttables.h>


namespace
{
QHash<FT_Encoding, QString>& encodingNames();
}


CharMapInfo::CharMapInfo(int index,
                         FT_CharMap cmap)
: index(index),
  ptr(cmap),
  encoding(cmap->encoding),
  platformID(cmap->platform_id),
  encodingID(cmap->encoding_id),
  formatID(FT_Get_CMap_Format(cmap)),
  languageID(FT_Get_CMap_Language_ID(cmap)),
  maxIndex(-1)
{
  auto& names = encodingNames();
  auto it = names.find(encoding);
  if (it == names.end())
    encodingName = &names[static_cast<FT_Encoding>(FT_ENCODING_OTHER)];
  else
    encodingName = &it.value();

  if (encoding != FT_ENCODING_OTHER)
    maxIndex = computeMaxIndex();
}


QString
CharMapInfo::stringifyIndex(int code,
                            int idx)
{
  return QString("CharCode: %1 (glyph idx %2)")
           .arg(stringifyIndexShort(code))
           .arg(idx);
}


QString
CharMapInfo::stringifyIndexShort(int code)
{
  return (encoding == FT_ENCODING_UNICODE ? "U+" : "0x")
         + QString::number(code, 16).rightJustified(4, '0').toUpper();
}


int
CharMapInfo::computeMaxIndex()
{
  int result;
  switch (encoding)
  {
  case FT_ENCODING_UNICODE:
    result = maxIndexForFaceAndCharMap(ptr, 0x110000) + 1;
    break;

  case FT_ENCODING_ADOBE_LATIN_1:
  case FT_ENCODING_ADOBE_STANDARD:
  case FT_ENCODING_ADOBE_EXPERT:
  case FT_ENCODING_ADOBE_CUSTOM:
  case FT_ENCODING_APPLE_ROMAN:
    result = 0x100;
    break;

  case FT_ENCODING_MS_SYMBOL:
    // Some fonts use range 0x00-0x100, others have 0xF000-0xF0FF.
    result = maxIndexForFaceAndCharMap(ptr, 0x10000) + 1;
    break;

  default:
    // Some encodings can reach > 0x10000, for example GB 18030.
    result = maxIndexForFaceAndCharMap(ptr, 0x110000) + 1;
  }

  return result;
}


int
CharMapInfo::maxIndexForFaceAndCharMap(FT_CharMap charMap,
                                       unsigned maxIn)
{
  // Code adopted from `ftcommon.c`.
  // This never overflows since no format here exceeds INT_MAX...
  FT_ULong min = 0;
  FT_ULong max = maxIn;
  FT_UInt glyphIndex;
  FT_Face face = charMap->face;

  if (FT_Set_Charmap(face, charMap))
    return -1;

  do
  {
    FT_ULong mid = (min + max) >> 1;
    FT_ULong res = FT_Get_Next_Char(face, mid, &glyphIndex);

    if (glyphIndex)
      min = res;
    else
    {
      max = mid;

      // Once moved, it helps to advance `min` through sparse regions.
      if (min)
      {
        res = FT_Get_Next_Char(face, min, &glyphIndex);

        if (glyphIndex)
          min = res;
        else
          max = min; // Found it.
      }
    }
  } while (max > min);

  return static_cast<int>(max);
}


namespace
{
// Mapping for `FT_Encoding` is placed here since it's only for the charmap.
QHash<FT_Encoding, QString> encodingNamesCache;

QHash<FT_Encoding, QString>&
encodingNames()
{
  if (encodingNamesCache.empty())
  {
    encodingNamesCache[static_cast<FT_Encoding>(FT_ENCODING_OTHER)]
     = "Unknown Encoding";
    encodingNamesCache[FT_ENCODING_NONE] = "No Encoding";
    encodingNamesCache[FT_ENCODING_MS_SYMBOL] = "MS Symbol (symb)";
    encodingNamesCache[FT_ENCODING_UNICODE] = "Unicode (unic)";
    encodingNamesCache[FT_ENCODING_SJIS] = "Shift JIS (sjis)";
    encodingNamesCache[FT_ENCODING_PRC] = "PRC/GB 18030 (gb)";
    encodingNamesCache[FT_ENCODING_BIG5] = "Big5 (big5)";
    encodingNamesCache[FT_ENCODING_WANSUNG] = "Wansung (wans)";
    encodingNamesCache[FT_ENCODING_JOHAB] = "Johab (joha)";
    encodingNamesCache[FT_ENCODING_ADOBE_STANDARD] = "Adobe Standard (ADOB)";
    encodingNamesCache[FT_ENCODING_ADOBE_EXPERT] = "Adobe Expert (ADBE)";
    encodingNamesCache[FT_ENCODING_ADOBE_CUSTOM] = "Adobe Custom (ADBC)";
    encodingNamesCache[FT_ENCODING_ADOBE_LATIN_1] = "Latin 1 (lat1)";
    encodingNamesCache[FT_ENCODING_OLD_LATIN_2] = "Latin 2 (lat2)";
    encodingNamesCache[FT_ENCODING_APPLE_ROMAN] = "Apple Roman (armn)";
  }

  return encodingNamesCache;
}
}


// end of charmap.cpp
