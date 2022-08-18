// fontinfo.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "fontinfo.hpp"

#include "engine.hpp"

#include <unordered_map>
#include <utility>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#else
#include <QStringConverter>
#include <QByteArrayView>
#endif
#include <freetype/ftmodapi.h>
#include <freetype/ttnameid.h>
#include <freetype/tttables.h>


void
SFNTName::get(Engine* engine,
  std::vector<SFNTName>& list)
{
  auto size = engine->currentFtSize();
  if (!size || !FT_IS_SFNT(size->face))
  {
    list.clear();
    return;
  }

  auto face = size->face;
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
    if (platformID == TT_MAC_ID_ROMAN)
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


FontBasicInfo
FontBasicInfo::get(Engine* engine)
{
  auto fontIndex = engine->currentFontIndex();
  if (fontIndex < 0)
    return {};
  FontBasicInfo result;
  result.numFaces = engine->numberOfFaces(fontIndex);

  engine->reloadFont();
  auto size = engine->currentFtSize();
  if (!size)
    return result;

  auto face = size->face;
  if (face->family_name)
    result.familyName = QString(face->family_name);
  if (face->style_name)
    result.styleName = QString(face->style_name);

  auto psName = FT_Get_Postscript_Name(face);
  if (psName)
    result.postscriptName = QString(psName);

  auto head = static_cast<TT_Header*>(FT_Get_Sfnt_Table(face, FT_SFNT_HEAD));
  if (head)
  {
    uint64_t createdTimestamp
      = head->Created[1] | static_cast<uint64_t>(head->Created[0]) << 32;
    uint64_t modifiedTimestamp
      = head->Modified[1] | static_cast<uint64_t>(head->Modified[0]) << 32;
    
    result.createdTime
      = QDateTime::fromSecsSinceEpoch(createdTimestamp, Qt::OffsetFromUTC)
          .addSecs(-2082844800);
    result.modifiedTime
      = QDateTime::fromSecsSinceEpoch(modifiedTimestamp, Qt::OffsetFromUTC)
          .addSecs(-2082844800);

    auto revDouble = head->Font_Revision / 65536.0;
    if (head->Font_Revision & 0xFFC0)
      result.revision = QString::number(revDouble, 'g', 4);
    else
      result.revision = QString::number(revDouble, 'g', 2);
  }

  return result;
}


FontTypeEntries
FontTypeEntries::get(Engine* engine)
{
  engine->reloadFont();
  auto size = engine->currentFtSize();
  if (!size)
    return {};

  auto face = size->face;

  FontTypeEntries result = {};
  result.driverName = QString(FT_FACE_DRIVER_NAME(face));
  result.sfnt = FT_IS_SFNT(face);
  result.scalable = FT_IS_SCALABLE(face);
  if (result.scalable)
    result.mmgx = FT_HAS_MULTIPLE_MASTERS(face);
  else
    result.mmgx = false;
  result.fixedSizes = FT_HAS_FIXED_SIZES(face);
  result.hasHorizontal = FT_HAS_HORIZONTAL(face);
  result.hasVertical = FT_HAS_VERTICAL(face);
  result.fixedWidth = FT_IS_FIXED_WIDTH(face);
  result.glyphNames = FT_HAS_GLYPH_NAMES(face);

  if (result.scalable)
  {
    result.emSize = face->units_per_EM;
    result.globalBBox = face->bbox;
    result.ascender = face->ascender;
    result.descender = face->descender;
    result.height = face->height;
    result.maxAdvanceWidth = face->max_advance_width;
    result.maxAdvanceHeight = face->max_advance_height;
    result.underlinePos = face->underline_position;
    result.underlineThickness = face->underline_thickness;
  }

  return result;
}


bool
operator==(const PS_FontInfoRec& lhs,
           const PS_FontInfoRec& rhs)
{
  // XXX: possible security flaw with `strcmp`?
  return strcmp(lhs.version, rhs.version) == 0
         && strcmp(lhs.notice, rhs.notice) == 0
         && strcmp(lhs.full_name, rhs.full_name) == 0
         && strcmp(lhs.family_name, rhs.family_name) == 0
         && strcmp(lhs.weight, rhs.weight) == 0
         && lhs.italic_angle == rhs.italic_angle
         && lhs.is_fixed_pitch == rhs.is_fixed_pitch
         && lhs.underline_position == rhs.underline_position
         && lhs.underline_thickness == rhs.underline_thickness;
}


bool
operator==(const PS_PrivateRec& lhs,
           const PS_PrivateRec& rhs)
{
  return lhs.unique_id == rhs.unique_id
         && lhs.lenIV == rhs.lenIV
         && lhs.num_blue_values == rhs.num_blue_values
         && lhs.num_other_blues == rhs.num_other_blues
         && lhs.num_family_blues == rhs.num_family_blues
         && lhs.num_family_other_blues == rhs.num_family_other_blues
         && std::equal(std::begin(lhs.blue_values), std::end(lhs.blue_values),
                       std::begin(rhs.blue_values))
         && std::equal(std::begin(lhs.other_blues), std::end(lhs.other_blues),
                       std::begin(rhs.other_blues))
         && std::equal(std::begin(lhs.family_blues), std::end(lhs.family_blues),
                       std::begin(rhs.family_blues))
         && std::equal(std::begin(lhs.family_other_blues),
                       std::end(lhs.family_other_blues),
                       std::begin(rhs.family_other_blues))
         && lhs.blue_scale == rhs.blue_scale
         && lhs.blue_shift == rhs.blue_shift
         && lhs.blue_fuzz == rhs.blue_fuzz
         && std::equal(std::begin(lhs.standard_width), 
                       std::end(lhs.standard_width),
                       std::begin(rhs.standard_width))
         && std::equal(std::begin(lhs.standard_height),
                       std::end(lhs.standard_height),
                       std::begin(rhs.standard_height))
         && lhs.num_snap_widths == rhs.num_snap_widths
         && lhs.num_snap_heights == rhs.num_snap_heights
         && lhs.force_bold == rhs.force_bold
         && lhs.round_stem_up == rhs.round_stem_up
         && std::equal(std::begin(lhs.snap_widths), std::end(lhs.snap_widths),
                       std::begin(rhs.snap_widths))
         && std::equal(std::begin(lhs.snap_heights), std::end(lhs.snap_heights),
                       std::begin(rhs.snap_heights))
         && lhs.expansion_factor == rhs.expansion_factor
         && lhs.language_group == rhs.language_group
         && lhs.password == rhs.password
         && std::equal(std::begin(lhs.min_feature), std::end(lhs.min_feature),
                       std::begin(rhs.min_feature));
}


bool
FontFixedSize::get(Engine* engine,
                   std::vector<FontFixedSize>& list,
                   const std::function<void()>& onUpdateNeeded)
{
  engine->reloadFont();
  auto size = engine->currentFtSize();
  if (!size)
  {
    if (list.empty())
      return false;

    onUpdateNeeded();
    list.clear();
    return true;
  }

  auto face = size->face;
  auto changed = false;
  if (list.size() != static_cast<size_t>(face->num_fixed_sizes))
  {
    changed = true;
    onUpdateNeeded();
    list.resize(face->num_fixed_sizes);
  }

  for (int i = 0; i < face->num_fixed_sizes; ++i)
  {
    FontFixedSize ffs = {};
    auto bSize = face->available_sizes + i;
    ffs.height = bSize->height;
    ffs.width  = bSize->width;
    ffs.size   = bSize->size / 64.0;
    ffs.xPpem  = bSize->x_ppem / 64.0;
    ffs.yPpem  = bSize->y_ppem / 64.0;
    if (ffs != list[i])
    {
      
      if (!changed)
      {
        onUpdateNeeded();
        changed = true;
      }
      
      list[i] = ffs;
    }
  }

  return changed;
}


// end of fontinfo.cpp