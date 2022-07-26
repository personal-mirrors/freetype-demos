// paletteinfo.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include <QString>

#include <freetype/freetype.h>
#include <freetype/ftcolor.h>
#include <freetype/ftsnames.h>

struct PaletteInfo
{
  int index;
  QString name;

  PaletteInfo(FT_Face face, FT_Palette_Data& data, int index);
};


// end of paletteinfo.hpp
