// fontinfo.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "fontinfo.hpp"

#include "engine.hpp"

#include <freetype/ftmodapi.h>
#include <freetype/tttables.h>


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
