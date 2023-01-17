// glyphoutline.hpp

// Copyright (C) 2016-2023 by
// Werner Lemberg.


#pragma once

#include <QGraphicsItem>
#include <QPainterPath>
#include <QPen>

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>


class GlyphOutline
: public QGraphicsItem
{
public:
  GlyphOutline(const QPen& pen,
               FT_Glyph glyph);
  QRectF boundingRect() const override;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

private:
  QPen outlinePen_;
  QPainterPath path_;
  QRectF boundingRect_;
};


// This class is common for all classes holding an outline.
// However, clss `GlyphOutline` itself doesn't need to hold the outline...

class GlyphUsingOutline
: public QGraphicsItem
{
public:
  GlyphUsingOutline(FT_Library library,
                    FT_Glyph glyph);
  ~GlyphUsingOutline() override;
  QRectF boundingRect() const override;

protected:
  FT_Library library_;
  FT_Outline outline_;
  bool outlineValid_;
  QRectF boundingRect_;
};


// end of glyphoutline.hpp
