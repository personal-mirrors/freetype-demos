// paletteinfo.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once

#include <vector>

#include <QString>

#include <freetype/freetype.h>
#include <freetype/ftcolor.h>


struct SFNTName;

struct PaletteInfo
{
  int index;
  QString name;

  PaletteInfo(FT_Face face,
              FT_Palette_Data& data,
              int index,
              std::vector<SFNTName> const* sfntNames);
};


// end of paletteinfo.hpp
