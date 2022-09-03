// glyphpoints.hpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#pragma once

#include <QGraphicsItem>
#include <QPen>

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>


class GlyphPoints
: public QGraphicsItem
{
public:
  GlyphPoints(const QPen& onPen,
              const QPen& offPen,
              FT_Glyph glyph);
  QRectF boundingRect() const override;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

private:
  QPen onPen_;
  QPen offPen_;
  FT_Outline* outline_;
  QRectF boundingRect_;
};


// end of glyphpoints.hpp
