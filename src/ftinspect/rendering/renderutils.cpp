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


FT_Bitmap
cloneBitmap(FT_Library library,
            FT_Bitmap* src)
{
  FT_Bitmap target = *src;
  target.buffer = NULL;
  target.palette = NULL;
  FT_Bitmap_Init(&target);
  FT_Bitmap_Copy(library, src, &target);
  return target;
}


void
transformOutlineToOrigin(FT_Outline* outline,
                         FT_BBox* outControlBox)
{
  FT_Pos x, y;
  computeTransformationToOrigin(outline,
                                &x, &y,
                                outControlBox);
  FT_Outline_Translate(outline, x, y);
}


void computeTransformationToOrigin(FT_Outline* outline,
                                   FT_Pos* outXOffset,
                                   FT_Pos* outYOffset,
                                   FT_BBox* outControlBox)
{
  FT_BBox cbox;
  FT_Outline_Get_CBox(outline, &cbox);

  cbox.xMin &= ~63;
  cbox.yMin &= ~63;
  cbox.xMax = (cbox.xMax + 63) & ~63;
  cbox.yMax = (cbox.yMax + 63) & ~63;
  // we shift the outline to the origin for rendering later on
  if (outXOffset)
    *outXOffset = -cbox.xMin;
  if (outYOffset)
    *outYOffset = -cbox.yMin;
  if (outControlBox)
    *outControlBox = cbox;
}


// end of renderutils.cpp
