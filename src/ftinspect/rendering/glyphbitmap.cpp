// glyphbitmap.cpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#include "glyphbitmap.hpp"

#include "renderutils.hpp"

#include <cmath>
#include <QPainter>
#include <QStyleOptionGraphicsItem>


GlyphBitmap::GlyphBitmap(FT_Outline* outline,
                         FT_Library lib,
                         FT_Pixel_Mode pxlMode,
                         const QVector<QRgb>& monoColorTbl,
                         const QVector<QRgb>& grayColorTbl)
: library_(lib),
  pixelMode_(pxlMode),
  monoColorTable_(monoColorTbl),
  grayColorTable_(grayColorTbl)
{
  // make a copy of the outline since we are going to manipulate it
  FT_BBox cbox;
  transformed_ = transformOutlineToOrigin(lib, outline, &cbox);
  boundingRect_.setCoords(cbox.xMin / 64, -cbox.yMax / 64,
                  cbox.xMax / 64, -cbox.yMin / 64);
}


GlyphBitmap::~GlyphBitmap()
{
  FT_Outline_Done(library_, &transformed_);
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
  FT_Bitmap bitmap;

  int height = static_cast<int>(ceil(boundingRect_.height()));
  int width = static_cast<int>(ceil(boundingRect_.width()));
  QImage::Format format = QImage::Format_Indexed8;

  // XXX cover LCD and color
  if (pixelMode_ == FT_PIXEL_MODE_MONO)
    format = QImage::Format_Mono;

  QImage image(QSize(width, height), format);

  if (pixelMode_ == FT_PIXEL_MODE_MONO)
    image.setColorTable(monoColorTable_);
  else
    image.setColorTable(grayColorTable_);

  image.fill(0);

  bitmap.rows = static_cast<unsigned int>(height);
  bitmap.width = static_cast<unsigned int>(width);
  bitmap.buffer = image.bits();
  bitmap.pitch = image.bytesPerLine();
  bitmap.pixel_mode = pixelMode_;

  FT_Error error = FT_Outline_Get_Bitmap(library_,
                                         &transformed_,
                                         &bitmap);
  if (error)
  {
    // XXX error handling
    return;
  }

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

  for (int x = 0; x < image.width(); x++)
    for (int y = 0; y < image.height(); y++)
    {
      // be careful not to lose the alpha channel
      QRgb p = image.pixel(x, y);
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
