// glyphoutline.cpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#include "glyphoutline.hpp"

#include <QPainter>


extern "C" {

// vertical font coordinates are bottom-up,
// while Qt uses top-down

static int
moveTo(const FT_Vector* to,
       void* user)
{
  QPainterPath* path = static_cast<QPainterPath*>(user);

  path->moveTo(qreal(to->x) / 64,
               -qreal(to->y) / 64);

  return 0;
}


static int
lineTo(const FT_Vector* to,
       void* user)
{
  QPainterPath* path = static_cast<QPainterPath*>(user);

  path->lineTo(qreal(to->x) / 64,
               -qreal(to->y) / 64);

  return 0;
}


static int
conicTo(const FT_Vector* control,
        const FT_Vector* to,
        void* user)
{
  QPainterPath* path = static_cast<QPainterPath*>(user);

  path->quadTo(qreal(control->x) / 64,
               -qreal(control->y) / 64,
               qreal(to->x) / 64,
               -qreal(to->y) / 64);

  return 0;
}


static int
cubicTo(const FT_Vector* control1,
        const FT_Vector* control2,
        const FT_Vector* to,
        void* user)
{
  QPainterPath* path = static_cast<QPainterPath*>(user);

  path->cubicTo(qreal(control1->x) / 64,
                -qreal(control1->y) / 64,
                qreal(control2->x) / 64,
                -qreal(control2->y) / 64,
                qreal(to->x) / 64,
                -qreal(to->y) / 64);

  return 0;
}


static FT_Outline_Funcs outlineFuncs =
{
  moveTo,
  lineTo,
  conicTo,
  cubicTo,
  0, // no shift
  0  // no delta
};

} // extern "C"


GlyphOutline::GlyphOutline(const QPen& pen,
                           FT_Glyph glyph)
: outlinePen_(pen)
{
  if (glyph->format != FT_GLYPH_FORMAT_OUTLINE)
  {
    outline_ = NULL;
    return;
  }
  outline_ = &reinterpret_cast<FT_OutlineGlyph>(glyph)->outline;

  FT_BBox cbox;

  qreal halfPenWidth = outlinePen_.widthF();

  FT_Outline_Get_CBox(outline_, &cbox);

  boundingRect_.setCoords(qreal(cbox.xMin) / 64 - halfPenWidth,
                  -qreal(cbox.yMax) / 64 - halfPenWidth,
                  qreal(cbox.xMax) / 64 + halfPenWidth,
                  -qreal(cbox.yMin) / 64 + halfPenWidth);
}


QRectF
GlyphOutline::boundingRect() const
{
  return boundingRect_;
}


void
GlyphOutline::paint(QPainter* painter,
                    const QStyleOptionGraphicsItem*,
                    QWidget*)
{
  if (!outline_)
    return;
  painter->setPen(outlinePen_);

  QPainterPath path;
  FT_Outline_Decompose(outline_, &outlineFuncs, &path);

  painter->drawPath(path);
}


// end of glyphoutline.cpp