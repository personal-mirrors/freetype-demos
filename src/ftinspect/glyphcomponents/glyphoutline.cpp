// glyphoutline.cpp

// Copyright (C) 2016-2023 by
// Werner Lemberg.


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
    return;
  auto outline = &reinterpret_cast<FT_OutlineGlyph>(glyph)->outline;
  FT_Outline_Decompose(outline, &outlineFuncs, &path_);

  FT_BBox cbox;

  qreal halfPenWidth = outlinePen_.widthF();

  FT_Outline_Get_CBox(outline, &cbox);

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
  painter->setPen(outlinePen_);
  painter->drawPath(path_);
}


GlyphUsingOutline::GlyphUsingOutline(FT_Library library,
                                     FT_Glyph glyph)
: library_(library)
{
  if (glyph->format != FT_GLYPH_FORMAT_OUTLINE)
  {
    outlineValid_ = false;
    return;
  }

  auto outline = &reinterpret_cast<FT_OutlineGlyph>(glyph)->outline;

  FT_BBox cbox;
  FT_Outline_Get_CBox(outline, &cbox);
  outlineValid_ = true;
  FT_Outline_New(library, static_cast<unsigned int>(outline->n_points),
                 outline->n_contours, &outline_);
  FT_Outline_Copy(outline, &outline_);

  // XXX fix bRect size
  boundingRect_.setCoords(qreal(cbox.xMin) / 64, -qreal(cbox.yMax) / 64,
                          qreal(cbox.xMax) / 64, -qreal(cbox.yMin) / 64);
}


GlyphUsingOutline::~GlyphUsingOutline()
{
  if (outlineValid_)
    FT_Outline_Done(library_, &outline_);
}


QRectF
GlyphUsingOutline::boundingRect() const
{
  return boundingRect_;
}


// end of glyphoutline.cpp
