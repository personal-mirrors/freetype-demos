// renderutils.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include <freetype/ftoutln.h>

// The constructed `outline` must be freed by the caller
FT_Outline transformOutlineToOrigin(FT_Library library, 
                                    FT_Outline* outline,
                                    FT_BBox* outControlBox);


// end of renderutils.hpp
