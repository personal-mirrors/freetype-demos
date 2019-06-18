#pragma once

#include <QGraphicsItem>
#include <QPen>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H


class GlyphSegment
: public QGraphicsItem
{
public:
  GlyphSegment(const QPen& segmentPen,
               const QPen& bluezonePen,
               FT_Size ftsize);
  QRectF boundingRect() const;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget);

private:
  QPen segmentPen;
  QPen bluezonePen;
  FT_Size ftsize;
  QRectF bRect;
};


// end of glyphsegment.hpp