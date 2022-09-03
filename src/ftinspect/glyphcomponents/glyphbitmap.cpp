// glyphbitmap.cpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#include "glyphbitmap.hpp"

#include "../engine/engine.hpp"

#include <cmath>
#include <utility>
#include <qevent.h>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <freetype/ftbitmap.h>


GlyphBitmap::GlyphBitmap(QImage* image,
                         QRect rect)
: image_(image),
  boundingRect_(rect)
{

}


GlyphBitmap::GlyphBitmap(int glyphIndex, 
                         FT_Glyph glyph,
                         Engine* engine)
{
  QRect bRect;
  image_ = NULL; // TODO: refactr Engine
  //image_ = engine->renderingEngine()->tryDirectRenderColorLayers(glyphIndex,
  //                                                               &bRect, true);

  //if (!image_)
  //  image_ = engine->renderingEngine()->convertGlyphToQImage(glyph, &bRect, 
  //                                                           true);
  boundingRect_ = bRect; // QRect to QRectF
}


GlyphBitmap::~GlyphBitmap()
{
  delete image_;
}

QRectF
GlyphBitmap::boundingRect() const
{
  return boundingRect_;
}


void
GlyphBitmap::paint(QPainter* painter,
                   const QStyleOptionGraphicsItem* option,
                   QWidget*)
{
  if (!image_)
    return;
  
  // `drawImage' doesn't work as expected:
  // the larger the zoom, the more the pixel rectangle positions
  // deviate from the grid lines
#if 0
  painter->drawImage(QPoint(bRect.left(), bRect.top()),
                     image.convertToFormat(
                       QImage::Format_ARGB32_Premultiplied));
#else
  const qreal lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(
    painter->worldTransform());

  painter->setPen(Qt::NoPen);

  for (int x = 0; x < image_->width(); x++)
    for (int y = 0; y < image_->height(); y++)
    {
      // be careful not to lose the alpha channel
      QRgb p = image_->pixel(x, y);
      painter->fillRect(QRectF(x + boundingRect_.left() - 1 / lod / 2,
                               y + boundingRect_.top() - 1 / lod / 2,
                               1 + 1 / lod,
                               1 + 1 / lod),
                        QColor(qRed(p),
                               qGreen(p),
                               qBlue(p),
                               qAlpha(p)));
    }
#endif
}


// end of glyphbitmap.cpp
