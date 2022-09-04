// rendering.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "rendering.hpp"

#include <cmath>
#include <freetype/ftbitmap.h>

#include "engine.hpp"

RenderingEngine::RenderingEngine(Engine* engine)
: engine_(engine)
{
  setForeground(QColor(Qt::black).rgba());
  setBackground(QColor(Qt::white).rgba());
}


void
RenderingEngine::setForeground(QRgb foreground)
{
  if (foregroundTable_.size() != 256 || foreground != foregroundColor_)
  {
    foregroundColor_ = foreground;
    calculateForegroundTable();
  }
}


void
RenderingEngine::setBackground(QRgb background)
{
  if (foregroundTable_.size() != 256 || background != backgroundColor_)
  {
    backgroundColor_ = background;
    calculateForegroundTable();
  }
}


void
RenderingEngine::setGamma(double gamma)
{
  if (gamma_ == gamma)
    return;
  gamma_ = gamma;
  calculateForegroundTable();
}


void
RenderingEngine::calculateForegroundTable()
{
  foregroundTable_.resize(256);
  auto gamma = gamma_;

  // Yes I know this is horribly slow, but we're only calculating the table once
  // and can use it for all rendering if the color and gamma isn't changing.

  double br = std::pow(qRed(backgroundColor_) / 255.0, gamma);
  double bg = std::pow(qGreen(backgroundColor_) / 255.0, gamma);
  double bb = std::pow(qBlue(backgroundColor_) / 255.0, gamma);
  double invGamma = 1 / gamma;

  for (int i = 0; i <= 0xFF; i++)
  {
    double foreAlpha = i * qAlpha(foregroundColor_) / 255.0 / 255.0;
    double backAlpha = 1 - foreAlpha;
    double r = std::pow(qRed(foregroundColor_) / 255.0, gamma);
    double g = std::pow(qGreen(foregroundColor_) / 255.0, gamma);
    double b = std::pow(qBlue(foregroundColor_) / 255.0, gamma);

    r = br * backAlpha + r * foreAlpha;
    g = bg * backAlpha + g * foreAlpha;
    b = bb * backAlpha + b * foreAlpha;

    r = std::pow(r, invGamma);
    g = std::pow(g, invGamma);
    b = std::pow(b, invGamma);

    foregroundTable_[i]
        = qRgba(static_cast<int>(r * 255), 
                static_cast<int>(g * 255),
                static_cast<int>(b * 255), 
                255);
  }
}


bool
RenderingEngine::convertGlyphToBitmapGlyph(FT_Glyph src,
                      FT_Glyph* out)
{
  if (src->format == FT_GLYPH_FORMAT_BITMAP)
  {
    // No need to convert.
    *out = src;
    return false;
  }
  if (src->format != FT_GLYPH_FORMAT_OUTLINE)
  {
    *out = NULL;
    return false;
    // TODO support SVG
  }

  if (src->format == FT_GLYPH_FORMAT_OUTLINE)
  {
    FT_Glyph out2 = src;
    // This will create a new glyph object.
    auto error = FT_Glyph_To_Bitmap(&out2, 
                                    engine_->renderMode(),
                                    nullptr,
                                    false);
    if (error)
    {
      *out = NULL;
      return false;
    }
    *out = out2;
    return true;
  }

  *out = NULL;
  return false;
}


FT_Bitmap
RenderingEngine::convertBitmapTo8Bpp(FT_Bitmap* bitmap)
{
  FT_Bitmap out;
  out.buffer = NULL;
  // This will create a new bitmap object
  auto error = FT_Bitmap_Convert(engine_->ftLibrary(), bitmap, &out, 1);
  if (error)
  {
    // XXX handling?
  }
  return out;
}


QImage*
RenderingEngine::convertBitmapToQImage(FT_Bitmap* src)
{
  QImage* result = NULL;
  
  auto& bmap = *src;
  bool ownBitmap = false; // if true, we need to cleanup `bmap`

  int width = INT_MAX, height = INT_MAX;
  if (bmap.width < INT_MAX)
    width = static_cast<int>(bmap.width);
  if (bmap.rows < INT_MAX)
    height = static_cast<int>(bmap.rows);
  auto format = QImage::Format_Indexed8; // goto crossing init

  if (bmap.pixel_mode == FT_PIXEL_MODE_GRAY2
      || bmap.pixel_mode == FT_PIXEL_MODE_GRAY4)
  {
    bmap = convertBitmapTo8Bpp(&bmap);
    if (!bmap.buffer)
      goto cleanup;
    ownBitmap = true;
  }

  if (bmap.pixel_mode == FT_PIXEL_MODE_LCD)
    width /= 3;
  else if (bmap.pixel_mode == FT_PIXEL_MODE_LCD_V)
    height /= 3;

  switch (bmap.pixel_mode)
  {
  case FT_PIXEL_MODE_MONO:
    format = QImage::Format_Mono;
    break;
  case FT_PIXEL_MODE_GRAY:
    format = QImage::Format_Indexed8;
    break;
  case FT_PIXEL_MODE_BGRA:
    // XXX "ARGB" here means BGRA due to endianness - may be problematic
    // on big-endian machines
    format = QImage::Format_ARGB32_Premultiplied;
    break;
  case FT_PIXEL_MODE_LCD:
  case FT_PIXEL_MODE_LCD_V:
    format = QImage::Format_ARGB32;
    break;
  default:
    goto cleanup;
  }

  switch (bmap.pixel_mode) 
  {
  case FT_PIXEL_MODE_MONO:
  case FT_PIXEL_MODE_GRAY:
  case FT_PIXEL_MODE_BGRA:
    {
      QImage image(bmap.buffer, 
                   width, height, 
                   bmap.pitch, 
                   format);
      if (bmap.pixel_mode == FT_PIXEL_MODE_GRAY)
        image.setColorTable(foregroundTable_);
      else if (bmap.pixel_mode == FT_PIXEL_MODE_MONO)
      {
        image.setColorCount(2);
        image.setColor(0, static_cast<QRgb>(0)); // transparent
        image.setColor(1, foregroundTable_[0xFF]);
      }
      result = new QImage(image.copy());
      // Don't directly use `image` since we're destroying the `bmap`
    }
    break;
  case FT_PIXEL_MODE_LCD:;
    result = new QImage(width, height, format);
    //convertLCDToARGB(bmap, *result, engine_->lcdUsesBGR(), foregroundTable_);
    break;
  case FT_PIXEL_MODE_LCD_V:;
    result = new QImage(width, height, format);
    //convertLCDVToARGB(bmap, *result, engine_->lcdUsesBGR(), foregroundTable_);
    break;
  }

cleanup:
  if (ownBitmap)
    FT_Bitmap_Done(engine_->ftLibrary(), &bmap);

  return result;
}


QImage*
RenderingEngine::convertGlyphToQImage(FT_Glyph src,
                             QRect* outRect,
                             bool inverseRectY)
{
  FT_BitmapGlyph bitmapGlyph;
  bool ownBitmapGlyph
    = convertGlyphToBitmapGlyph(src, reinterpret_cast<FT_Glyph*>(&bitmapGlyph));
  if (!bitmapGlyph)
    return NULL;

  auto result = convertBitmapToQImage(&bitmapGlyph->bitmap);

  if (result && outRect)
  {
    outRect->setLeft(bitmapGlyph->left);
    if (inverseRectY)
      outRect->setTop(-bitmapGlyph->top);
    else
      outRect->setTop(bitmapGlyph->top);
    if (bitmapGlyph->bitmap.width < INT_MAX)
      outRect->setWidth(static_cast<int>(bitmapGlyph->bitmap.width));
    else
      outRect->setWidth(INT_MAX);

    if (bitmapGlyph->bitmap.rows < INT_MAX)
      outRect->setHeight(static_cast<int>(bitmapGlyph->bitmap.rows));
    else
      outRect->setHeight(INT_MAX);
  }

  if (ownBitmapGlyph)
    FT_Done_Glyph(reinterpret_cast<FT_Glyph>(bitmapGlyph));

  return result;
}


QPoint
RenderingEngine::computeGlyphOffset(FT_Glyph glyph, bool inverseY)
{
  if (glyph->format == FT_GLYPH_FORMAT_OUTLINE)
  {
    FT_BBox cbox;
    FT_Outline_Get_CBox(&reinterpret_cast<FT_OutlineGlyph>(glyph)->outline, 
                        &cbox);
    cbox.xMin &= ~63;
    cbox.yMin &= ~63;
    cbox.xMax = (cbox.xMax + 63) & ~63;
    cbox.yMax = (cbox.yMax + 63) & ~63;
    if (inverseY)
      cbox.yMax = -cbox.yMax;
    return { static_cast<int>(cbox.xMin / 64),
             static_cast<int>(cbox.yMax / 64) };
  }
  if (glyph->format == FT_GLYPH_FORMAT_BITMAP)
  {
    auto bg = reinterpret_cast<FT_BitmapGlyph>(glyph);
    if (inverseY)
      return { bg->left, -bg->top };
    return { bg->left, bg->top };
  }

  return {};
}


QImage*
RenderingEngine::tryDirectRenderColorLayers(int glyphIndex,
                                   QRect* outRect,
                                   bool inverseRectY)
{
  return NULL; // TODO: impl
}


// end of rendering.cpp
