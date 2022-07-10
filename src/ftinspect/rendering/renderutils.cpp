// renderutils.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "renderutils.hpp"

FT_Outline
transformOutlineToOrigin(FT_Library library, 
                         FT_Outline* outline,
                         FT_BBox* outControlBox)
{
  FT_Outline transformed;
  FT_Outline_New(library,
                 static_cast<unsigned int>(outline->n_points),
                 outline->n_contours, &transformed);
  FT_Outline_Copy(outline, &transformed);

  FT_BBox cbox;
  FT_Outline_Get_CBox(outline, &cbox);

  cbox.xMin &= ~63;
  cbox.yMin &= ~63;
  cbox.xMax = (cbox.xMax + 63) & ~63;
  cbox.yMax = (cbox.yMax + 63) & ~63;
  // we shift the outline to the origin for rendering later on
  FT_Outline_Translate(&transformed, -cbox.xMin, -cbox.yMin);

  if (outControlBox)
    *outControlBox = cbox;
  return transformed;
}


// end of renderutils.cpp
