// mmgx.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "engine.hpp"
#include "mmgx.hpp"

#include <freetype/ftmm.h>


MMGXState
MMGXAxisInfo::get(Engine* engine,
                  std::vector<MMGXAxisInfo>& infos)
{
  auto face = engine->currentFallbackFtFace();
  if (!face)
  {
    infos.clear();
    return MMGXState::NoMMGX;
  }

  if (!FT_HAS_MULTIPLE_MASTERS(face))
  {
    infos.clear();
    return MMGXState::NoMMGX;
  }

  FT_Multi_Master dummy;
  auto error = FT_Get_Multi_Master(face, &dummy);
  auto state = error ? MMGXState::GX_OVF : MMGXState::MM;

  FT_MM_Var* mm;
  if (FT_Get_MM_Var(face, &mm))
  {
    infos.clear();
    return state;
  }

  infos.resize(mm->num_axis);

  auto& sfnt = engine->currentFontSFNTNames();
  for (unsigned int i = 0; i < mm->num_axis; ++i)
  {
    auto& axis = mm->axis[i];
    auto& info = infos[i];
    info.maximum = axis.maximum / 65536.0;
    info.minimum = axis.minimum / 65536.0;
    info.def = axis.def / 65536.0;
    info.tag = axis.tag;
    info.isMM = state == MMGXState::MM;

    unsigned int flags = 0;
    FT_Get_Var_Axis_Flags(mm, i, &flags);
    info.hidden = (flags & FT_VAR_AXIS_FLAG_HIDDEN) != 0;

    auto nameSet = false;
    if (state == MMGXState::GX_OVF)
    {
      auto strid = mm->axis[i].strid;
      for (auto& obj : sfnt)
      {
        if (obj.nameID == strid && obj.strValid)
        {
          info.name = obj.str;
          nameSet = true;
          break;
        }
      }
    }

    // XXX security flaw
    if (!nameSet)
      info.name = QString(axis.name);
  }

  FT_Done_MM_Var(face->glyph->library, mm);

  return state;
}


// end of mmgx.cpp
