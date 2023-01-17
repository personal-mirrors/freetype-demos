// glyphpoints.hpp

// Copyright (C) 2016-2023 by
// Werner Lemberg.


#pragma once

#include "glyphoutline.hpp"

#include <QGraphicsItem>
#include <QPen>

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>


class GlyphPoints
: public GlyphUsingOutline
{
public:
  GlyphPoints(FT_Library library,
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


// end of glyphpoints.hpp
