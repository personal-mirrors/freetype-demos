// glyphpointnumbers.hpp

// Copyright (C) 2016-2023 by
// Werner Lemberg.


#pragma once

#include "glyphoutline.hpp"

class GlyphPointNumbers
: public GlyphUsingOutline
{
public:
  GlyphPointNumbers(FT_Library library,
                    const QPen& onPen,
                    const QPen& offPen,
                    FT_Glyph glyph);
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

private:
  QPen onPen_;
  QPen offPen_;
};


// end of glyphpointnumbers.hpp
