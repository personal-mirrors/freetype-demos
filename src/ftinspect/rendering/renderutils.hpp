// renderutils.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>

// The constructed `outline` must be freed by the caller
FT_Outline cloneOutline(FT_Library library, FT_Outline* src);
FT_Glyph cloneGlyph(FT_Glyph src);

void transformOutlineToOrigin(FT_Outline* outline,
                              FT_BBox* outControlBox);

void computeTransformationToOrigin(FT_Outline* outline,
                                   FT_Pos* outXOffset,
                                   FT_Pos* outYOffset,
                                   FT_BBox* outControlBox);


// end of renderutils.hpp
