// glyphbitmap.cpp

// Copyright (C) 2016-2019 by Werner Lemberg.


#include "glyphbitmap.hpp"

#include <cmath>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QtDebug>


GlyphBitmap::GlyphBitmap(FT_Outline* outline,
                         FT_Library lib,
                         FT_Pixel_Mode pxlMode,
                         double gammaVal,
                         const QVector<QRgb>& monoColorTbl,
                         const QVector<QRgb>& grayColorTbl)
: library(lib),
  pixelMode(pxlMode),
  gamma(gammaVal),
  monoColorTable(monoColorTbl),
  grayColorTable(grayColorTbl)
{
  // make a copy of the outline since we are going to manipulate it
  FT_Outline_New(library,
                 static_cast<unsigned int>(outline->n_points),
                 outline->n_contours,
                 &transformed);
  FT_Outline_Copy(outline, &transformed);

  FT_BBox cbox;
  FT_Outline_Get_CBox(outline, &cbox);

  cbox.xMin &= ~63;
  cbox.yMin &= ~63;
  cbox.xMax = (cbox.xMax + 63) & ~63;
  cbox.yMax = (cbox.yMax + 63) & ~63;

  // we shift the outline to the origin for rendering later on
  FT_Outline_Translate(&transformed, -cbox.xMin, -cbox.yMin);

  bRect.setCoords(cbox.xMin / 64, -cbox.yMax / 64,
                  cbox.xMax / 64, -cbox.yMin / 64);
}


GlyphBitmap::~GlyphBitmap()
{
  FT_Outline_Done(library, &transformed);
}

QRectF
GlyphBitmap::boundingRect() const
{
  return bRect;
}


void
GlyphBitmap::paint(QPainter* painter,
                   const QStyleOptionGraphicsItem* option,
                   QWidget*)
{
  FT_Bitmap bitmap;

  if ( gamma <= 0 ) // special case for sRGB
  {
    gamma = 2.4;
  }

  int height = static_cast<int>(ceil(bRect.height()));
  int width = static_cast<int>(ceil(bRect.width()));
  QImage::Format format = QImage::Format_Indexed8;

  // XXX cover LCD and color
  if (pixelMode == FT_PIXEL_MODE_MONO)
    format = QImage::Format_Mono;

  QImage image(QSize(width, height), format);

  if (pixelMode == FT_PIXEL_MODE_MONO)
    image.setColorTable(monoColorTable);
  else
    image.setColorTable(grayColorTable);

  image.fill(0);

  bitmap.rows = static_cast<unsigned int>(height);
  bitmap.width = static_cast<unsigned int>(width);
  bitmap.buffer = image.bits();
  bitmap.pitch = image.bytesPerLine();
  bitmap.pixel_mode = pixelMode;

  FT_Error error = FT_Outline_Get_Bitmap(library,
                                         &transformed,
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
      const double r = qRed(p) / 255.0;
      const double g = qGreen(p) / 255.0;
      const double b = qBlue(p) / 255.0;
      const double a = qAlpha(p) / 255.0;
      painter->fillRect(QRectF(x + bRect.left() - 1 / lod / 2,
                               y + bRect.top() - 1 / lod / 2,
                               1 + 1 / lod,
                               1 + 1 / lod),
                        QColor(
                               255 * std::pow(r, 1/gamma),
                               255 * std::pow(g, 1/gamma),
                               255 * std::pow(b, 1/gamma),
                               255 * std::pow(a, 1/gamma)));
    }
#endif
}


// end of glyphbitmap.cpp
