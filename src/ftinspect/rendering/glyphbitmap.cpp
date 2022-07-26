// glyphbitmap.cpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#include "glyphbitmap.hpp"

#include "renderutils.hpp"
#include "../engine/engine.hpp"

#include <cmath>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <freetype/ftbitmap.h>


GlyphBitmap::GlyphBitmap(int glyphIndex, 
                         FT_Glyph glyph,
                         Engine* engine)
{
  QRect bRect;
  image_ = engine->tryDirectRenderColorLayers(glyphIndex, &bRect);
  if (image_)
  {
    bRect.setTop(-bRect.top());
    boundingRect_ = bRect; // QRect to QRectF
    return;
  }

  image_ = engine->convertGlyphToQImage(glyph, &bRect, true);
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
  const qreal lod = option->levelOfDetailFromTransform(
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
