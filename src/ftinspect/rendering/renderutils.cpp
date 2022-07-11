// renderutils.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "renderutils.hpp"

FT_Outline
cloneOutline(FT_Library library, 
             FT_Outline* src)
{
  FT_Outline transformed;
  FT_Outline_New(library, static_cast<unsigned int>(src->n_points),
                 src->n_contours, &transformed);
  FT_Outline_Copy(src, &transformed);
  return transformed;
}


FT_Glyph
cloneGlyph(FT_Glyph src)
{
  FT_Glyph target = NULL;
  FT_Glyph_Copy(src, &target);
  return target;
}


void
transformOutlineToOrigin(FT_Outline* outline,
                         FT_BBox* outControlBox)
{
  FT_BBox cbox;
  FT_Outline_Get_CBox(outline, &cbox);

  cbox.xMin &= ~63;
  cbox.yMin &= ~63;
  cbox.xMax = (cbox.xMax + 63) & ~63;
  cbox.yMax = (cbox.yMax + 63) & ~63;
  // we shift the outline to the origin for rendering later on
  FT_Outline_Translate(outline, -cbox.xMin, -cbox.yMin);

  if (outControlBox)
    *outControlBox = cbox;
}


// end of renderutils.cpp
