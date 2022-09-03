// glyphbitmap.hpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#pragma once

#include <QGraphicsItem>
#include <QPen>
#include <QPaintEvent>
#include <QWidget>

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>


class Engine;

class GlyphBitmap
: public QGraphicsItem
{
public:
  GlyphBitmap(QImage* image,
              QRect rect);
  GlyphBitmap(int glyphIndex,
              FT_Glyph glyph,
              Engine* engine);
  ~GlyphBitmap() override;
  QRectF boundingRect() const override;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

private:
  QImage* image_ = NULL;
  QRectF boundingRect_;
};


// end of glyphbitmap.hpp
