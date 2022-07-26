// paletteinfo.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "paletteinfo.hpp"

PaletteInfo::PaletteInfo(FT_Face face, 
                         FT_Palette_Data& data, 
                         int index)
: index(index)
{
  if (data.palette_name_ids)
  {
    auto id = data.palette_name_ids[index];
    FT_SfntName sname;
    FT_Get_Sfnt_Name(face, id, &sname);
    name = "(SFNT no impl)";
    // TODO: Get SFNT Name: After implemented SFNT names functionality.
  }
  else
  {
    name = "(unnamed)";
  }
}


// end of paletteinfo.cpp
