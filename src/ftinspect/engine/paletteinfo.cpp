// paletteinfo.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "paletteinfo.hpp"

//#include "fontinfo.hpp"

PaletteInfo::PaletteInfo(FT_Face face, 
                         FT_Palette_Data& data, 
                         int index,
                         std::vector<SFNTName> const* sfntNames)
: index(index)
{
  if (sfntNames && data.palette_name_ids)
  {
    auto id = data.palette_name_ids[index];
    name = "(unsupported)";
    //for (auto& obj : *sfntNames)
    //{
    //  if (obj.nameID == id && obj.strValid)
    //  {
    //    name = obj.str;
    //    break;
    //  }
    //}
  }
  else
    name = "(unnamed)";
}


// end of paletteinfo.cpp
