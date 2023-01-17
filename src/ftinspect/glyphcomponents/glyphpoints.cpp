// glyphpoints.cpp

// Copyright (C) 2016-2023 by
// Werner Lemberg.


#include "glyphpoints.hpp"

#include <QPainter>
#include <QStyleOptionGraphicsItem>


GlyphPoints::GlyphPoints(FT_Library library,
                         const QPen& onP,
                         const QPen& offP,
                         FT_Glyph glyph)
: GlyphUsingOutline(library, glyph),
  onPen_(onP),
  offPen_(offP)
{
}


void
GlyphPoints::paint(QPainter* painter,
                   const QStyleOptionGraphicsItem* option,
                   QWidget*)
{
  if (!outlineValid_)
    return;

  const qreal lod = option->levelOfDetailFromTransform(
                              painter->worldTransform());

  // Don't draw points if magnification is too small.
  if (lod >= 5)
  {
    // We want the same dot size regardless of the scaling;
    // for good optical results, the pen widths should be uneven integers.

    // Interestingly, using `drawPoint` doesn't work as expected:
    // the larger the zoom, the more horizontally stretched the dot appears.
#if 0
    qreal origOnPenWidth = onPen.widthF();
    qreal origOffPenWidth = offPen.widthF();

    onPen.setWidthF(origOnPenWidth / lod);
    offPen.setWidthF(origOffPenWidth / lod);

    for (int i = 0; i < outline->n_points; i++)
    {
      if (outline->tags[i] & FT_CURVE_TAG_ON)
        painter->setPen(onPen);
      else
        painter->setPen(offPen);

      painter->drawPoint(QPointF(qreal(outline->points[i].x) / 64,
                                 -qreal(outline->points[i].y) / 64));
    }

    onPen.setWidthF(origOnPenWidth);
    offPen.setWidthF(origOffPenWidth);
#else
    QBrush onBrush(onPen_.color());
    QBrush offBrush(offPen_.color());

    painter->setPen(Qt::NoPen);

    qreal onRadius = onPen_.widthF() / lod;
    qreal offRadius = offPen_.widthF() / lod;

    for (int i = 0; i < outline_.n_points; i++)
    {
      if (outline_.tags[i] & FT_CURVE_TAG_ON)
      {
        painter->setBrush(onBrush);
        painter->drawEllipse(QPointF(qreal(outline_.points[i].x) / 64,
                                     -qreal(outline_.points[i].y) / 64),
                             onRadius,
                             onRadius);
      }
      else
      {
        painter->setBrush(offBrush);
        painter->drawEllipse(QPointF(qreal(outline_.points[i].x) / 64,
                                     -qreal(outline_.points[i].y) / 64),
                             offRadius,
                             offRadius);
      }
    }
#endif
  }
}


// end of glyphpoints.cpp
