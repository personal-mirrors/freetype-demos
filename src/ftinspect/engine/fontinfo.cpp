// fontinfo.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "engine.hpp"
#include "fontinfo.hpp"

#include <map>
#include <memory>
#include <unordered_map>
#include <utility>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
# include <QTextCodec>
#else
# include <QByteArrayView>
# include <QStringConverter>
#endif

#include <freetype/ftmodapi.h>
#include <freetype/ttnameid.h>
#include <freetype/tttables.h>
#include <freetype/tttags.h>

#ifdef _MSC_VER // To use `intrin.h`.
# define WIN32_LEAN_AND_MEAN
# include <Windows.h>
# include <intrin.h>
#endif


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
        obj.langTag = utf16BEToQString(
                        reinterpret_cast<char*>(langTag.string),
                        langTag.string_len);
    }
  }
}


QString
SFNTName::sfntNameToQString(FT_SfntName const& sfntName,
                            bool* outSuccess)
{
  return sfntNameToQString(sfntName.platform_id,
                           sfntName.encoding_id,
                           reinterpret_cast<char const*>(sfntName.string),
                           sfntName.string_len,
                           outSuccess);
}


QString
SFNTName::sfntNameToQString(SFNTName const& sfntName,
                            bool* outSuccess)
{
  return sfntNameToQString(sfntName.platformID,
                           sfntName.encodingID,
                           sfntName.strBuf.data(),
                           sfntName.strBuf.size(),
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
      // TT_MS_ID_SYMBOL_CS is Unicode, similar to PID/EID=3/1.
    case TT_MS_ID_SYMBOL_CS:
    case TT_MS_ID_UNICODE_CS:
    case TT_MS_ID_UCS_4: // This is UTF-16LE as well, according to MS doc.
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
  auto face = engine->currentFallbackFtFace();
  if (!face)
    return result;

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
  auto face = engine->currentFallbackFtFace();
  if (!face)
    return {};

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
  auto face = engine->currentFallbackFtFace();
  if (!face)
  {
    if (list.empty())
      return false;

    onUpdateNeeded();
    list.clear();
    return true;
  }

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
    ffs.width = bSize->width;
    ffs.size = bSize->size / 64.0;
    ffs.xPpem = bSize->x_ppem / 64.0;
    ffs.yPpem = bSize->y_ppem / 64.0;
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


struct TTCHeaderRec
{
  uint32_t ttcTag;
  uint16_t majorVersion;
  uint16_t minorVersion;
  uint32_t numFonts;
};


struct SFNTHeaderRec
{
  uint32_t formatTag;
  uint16_t numTables;
  // There will be some padding, but it doesn't matter.
};


struct TTTableRec
{
  uint32_t tag;
  uint32_t checksum;
  uint32_t offset;
  uint32_t length;
};


uint32_t
bigEndianToNative(uint32_t n)
{
#ifdef _MSC_VER
# if REG_DWORD == REG_DWORD_LITTLE_ENDIAN
  return _byteswap_ulong(n);
# else
  return n;
# endif
#else
  auto np = reinterpret_cast<unsigned char*>(&n);

  return (static_cast<uint32_t>(np[0]) << 24)
         | (static_cast<uint32_t>(np[1]) << 16)
         | (static_cast<uint32_t>(np[2]) << 8)
         | (static_cast<uint32_t>(np[3]));
#endif
}


uint16_t
bigEndianToNative(uint16_t n)
{
#ifdef _MSC_VER
# if REG_DWORD == REG_DWORD_LITTLE_ENDIAN
  return _byteswap_ushort(n);
# else
  return n;
# endif
#else
  auto np = reinterpret_cast<unsigned char*>(&n);

  return static_cast<uint16_t>((static_cast<uint16_t>(np[0]) << 8)
                               | (static_cast<uint16_t>(np[1])));
#endif
}


void readSingleFace(QFile& file,
                    uint32_t offset,
                    unsigned faceIndex,
                    std::vector<TTTableRec>& tempTables,
                    std::map<unsigned long,
                    SFNTTableInfo>& result)
{
  if (!file.seek(offset))
    return;

  SFNTHeaderRec sfntHeader = {};
  if (file.read(reinterpret_cast<char*>(&sfntHeader),
                sizeof(SFNTHeaderRec))
      != sizeof(SFNTHeaderRec))
    return;
  sfntHeader.formatTag = bigEndianToNative(sfntHeader.formatTag);
  sfntHeader.numTables = bigEndianToNative(sfntHeader.numTables);

  unsigned short validEntries = sfntHeader.numTables;

  if (sfntHeader.formatTag != TTAG_OTTO)
  {
    // TODO check SFNT Header
    //checkSFNTHeader();
  }

  if (!file.seek(offset + 12))
    return;

  tempTables.resize(validEntries);
  auto desiredLen = static_cast<long long>(validEntries * sizeof(TTTableRec));
  auto readLen = file.read(reinterpret_cast<char*>(tempTables.data()),
                           desiredLen);
  if (readLen != desiredLen)
    return;

  for (auto& t : tempTables)
  {
    t.tag = bigEndianToNative(t.tag);
    t.offset = bigEndianToNative(t.offset);
    t.checksum = bigEndianToNative(t.checksum);
    t.length = bigEndianToNative(t.length);

    auto it = result.find(t.offset);
    if (it == result.end())
    {
      auto emplaced = result.emplace(t.offset, SFNTTableInfo());
      it = emplaced.first;

      auto& info = it->second;
      info.tag = t.tag;
      info.length = t.length;
      info.offset = t.offset;
      info.sharedFaces.emplace(faceIndex);
      info.valid = true;
    }
    else
    {
      it->second.sharedFaces.emplace(faceIndex);
      // TODO check
    }
  }
}


void
SFNTTableInfo::getForAll(Engine* engine,
                         std::vector<SFNTTableInfo>& infos)
{
  infos.clear();
  auto face = engine->currentFallbackFtFace();
  if (!face || !FT_IS_SFNT(face))
    return;

  auto index = engine->currentFontIndex();
  auto& mgr = engine->fontFileManager();
  if (index < 0 || index >= mgr.size())
    return;

  auto& fileInfo = mgr[index];
  QFile file(fileInfo.filePath());
  if (!file.open(QIODevice::ReadOnly))
    return;

  auto fileSize = file.size();
  if (fileSize < 12)
    return;

  std::vector<TTTableRec> tables;
  std::map<unsigned long, SFNTTableInfo> result;

  TTCHeaderRec ttcHeader = {};
  auto readLen = file.read(reinterpret_cast<char*>(&ttcHeader),
                           sizeof(TTCHeaderRec));

  if (readLen != sizeof(TTCHeaderRec))
    return;

  ttcHeader.ttcTag = bigEndianToNative(ttcHeader.ttcTag);
  ttcHeader.majorVersion = bigEndianToNative(ttcHeader.majorVersion);
  ttcHeader.minorVersion = bigEndianToNative(ttcHeader.minorVersion);
  ttcHeader.numFonts = bigEndianToNative(ttcHeader.numFonts);

  if (ttcHeader.ttcTag == TTAG_ttcf
      && (ttcHeader.majorVersion == 2 || ttcHeader.majorVersion == 1))
  {
    // Valid TTC file.
    std::unique_ptr<unsigned> offsets(new unsigned[ttcHeader.numFonts]);
    auto desiredLen = static_cast<long long>(ttcHeader.numFonts
                                             * sizeof(unsigned));
    readLen = file.read(reinterpret_cast<char*>(offsets.get()), desiredLen);
    if (readLen != desiredLen)
      return;

    for (unsigned faceIndex = 0;
         faceIndex < ttcHeader.numFonts;
         faceIndex++)
    {
      auto offset = bigEndianToNative(offsets.get()[faceIndex]);
      readSingleFace(file, offset, faceIndex, tables, result);
    }
  }
  else
  {
    // Not TTC file, try single SFNT.
    if (!file.seek(0))
      return;
    readSingleFace(file, 0, 0, tables, result);
  }

  infos.reserve(result.size());
  for (auto& pr : result)
    infos.emplace_back(std::move(pr.second));
}


FT_UInt16
readUInt16(void* ptr)
{
  return bigEndianToNative(*static_cast<uint16_t*>(ptr));
}


double
readF2Dot14(void* ptr)
{
  return static_cast<int16_t>(readUInt16(ptr)) / 16384.0;
}


void
CompositeGlyphInfo::get(Engine* engine,
                        std::vector<CompositeGlyphInfo>& list)
{
  list.clear();
  engine->reloadFont();
  auto face = engine->currentFallbackFtFace();
  if (!face || !FT_IS_SFNT(face))
  {
    if (list.empty())
      return;
  }

  // We are not using FreeType's subglyph APIs but directly reading from
  // the 'glyf' table since it is faster.
  auto head = static_cast<TT_Header*>(FT_Get_Sfnt_Table(face, FT_SFNT_HEAD));
  auto maxp
    = static_cast<TT_MaxProfile*>(FT_Get_Sfnt_Table(face, FT_SFNT_MAXP));
  if (!head || !maxp)
    return;

  FT_ULong locaLength = head->Index_To_Loc_Format ? 4 * maxp->numGlyphs + 4
                                                  : 2 * maxp->numGlyphs + 2;
  std::unique_ptr<unsigned char[]> locaBufferGuard(
    new unsigned char[locaLength]);
  auto offset = locaBufferGuard.get();
  auto error = FT_Load_Sfnt_Table(face, TTAG_loca, 0, offset, &locaLength);
  if (error)
    return;

  FT_ULong glyfLength = 0;
  error = FT_Load_Sfnt_Table(face, TTAG_glyf, 0, NULL, &glyfLength);
  if (error || !glyfLength)
    return;

  std::unique_ptr<unsigned char[]> glyfBufferGuard(
    new unsigned char[glyfLength]);
  auto buffer = glyfBufferGuard.get();
  error = FT_Load_Sfnt_Table(face, TTAG_glyf, 0, buffer, &glyfLength);
  if (error)
    return;

  for (size_t i = 0; i < maxp->numGlyphs; i++)
  {
    FT_UInt32 loc;
    FT_UInt32 end;

    if (head->Index_To_Loc_Format)
    {
      loc = static_cast<FT_UInt32>(offset[4 * i]) << 24
            | static_cast<FT_UInt32>(offset[4 * i + 1]) << 16
            | static_cast<FT_UInt32>(offset[4 * i + 2]) << 8
            | static_cast<FT_UInt32>(offset[4 * i + 3]);
      end = static_cast<FT_UInt32>(offset[4 * i + 4]) << 24
            | static_cast<FT_UInt32>(offset[4 * i + 5]) << 16
            | static_cast<FT_UInt32>(offset[4 * i + 6]) << 8
            | static_cast<FT_UInt32>(offset[4 * i + 7]);
    }
    else
    {
      loc = static_cast<FT_UInt32>(offset[2 * i]) << 9
            | static_cast<FT_UInt32>(offset[2 * i + 1]) << 1;
      end = static_cast<FT_UInt32>(offset[2 * i + 2]) << 9
            | static_cast<FT_UInt32>(offset[2 * i + 3]) << 1;
    }

    if (end > glyfLength)
      end = glyfLength;

    if (loc + 16 > end)
      continue;

    auto len = static_cast<FT_Int16>(readUInt16(buffer + loc));
    loc += 10;  // Skip header.
    if (len >= 0) // Not a composite one.
      continue;

    std::vector<SubGlyph> subglyphs;

    while (true)
    {
      if (loc + 6 > end)
        break;
      auto flags = readUInt16(buffer + loc);
      loc += 2;
      auto index = readUInt16(buffer + loc);
      loc += 2;
      FT_Int16 arg1, arg2;

      // https://docs.microsoft.com/en-us/typography/opentype/spec/glyf#composite-glyph-description
      if (flags & 0x0001)
      {
        arg1 = static_cast<FT_Int16>(readUInt16(buffer + loc));
        loc += 2;
        arg2 = static_cast<FT_Int16>(readUInt16(buffer + loc));
        loc += 2;
      }
      else
      {
        arg1 = buffer[loc];
        arg2 = buffer[loc + 1];
        loc += 2;
      }

      subglyphs.emplace_back(index, flags,
                             flags & 0x0002 ? SubGlyph::PT_Offset
                                            : SubGlyph::PT_Align,
                             std::pair<short, short>(arg1, arg2),
                             (flags & 0x0800) != 0);
      // TODO: Use "Default behavior" when neither SCALED_COMPONENT_OFFSET
      //       nor UNSCALED_COMPONENT_OFFSET are set.

      auto& glyph = subglyphs.back();
      if (flags & 0x0008)
      {
        glyph.transformationType = SubGlyph::TT_UniformScale;
        glyph.transformation[0] = readF2Dot14(buffer + loc);
        loc += 2;
      }
      else if (flags & 0x0040)
      {
        glyph.transformationType = SubGlyph::TT_XYScale;
        glyph.transformation[0] = readF2Dot14(buffer + loc);
        glyph.transformation[1] = readF2Dot14(buffer + loc + 2);
        loc += 4;
      }
      else if (flags & 0x0080)
      {
        glyph.transformationType = SubGlyph::TT_Matrix;
        glyph.transformation[0] = readF2Dot14(buffer + loc);
        glyph.transformation[1] = readF2Dot14(buffer + loc + 2);
        glyph.transformation[2] = readF2Dot14(buffer + loc + 4);
        glyph.transformation[3] = readF2Dot14(buffer + loc + 6);
        loc += 8;
      }
      else
      {
        glyph.transformationType = SubGlyph::TT_UniformScale;
        glyph.transformation[0] = 1.0;
      }

      if (!(flags & 0x0020))
        break;
    }

    list.emplace_back(static_cast<int>(i), std::move(subglyphs));
  }
}


// end of fontinfo.cpp
