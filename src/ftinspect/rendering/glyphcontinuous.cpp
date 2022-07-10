// glyphcontinuous.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "glyphcontinuous.hpp"

#include <cmath>
#include <QPainter>
#include <QWheelEvent>

#include "../engine/engine.hpp"
#include "../rendering/renderutils.hpp"


GlyphContinuous::GlyphContinuous(QWidget* parent, Engine* engine)
: QWidget(parent), engine_(engine)
{
  setAcceptDrops(false);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  graphicsDefault_ = GraphicsDefault::deafultInstance();
}


void
GlyphContinuous::paintEvent(QPaintEvent* event)
{
  QPainter painter;
  painter.begin(this);
  painter.fillRect(rect(), Qt::white);

  if (limitIndex_ > 0)
  {
    prePaint();

    switch (mode_)
    {
    case AllGlyphs:
      switch (modeAG_)
      {
      case AG_AllGlyphs:
        paintAGAllGlyphs(&painter);
        break;
        // TODO more modes
      case AG_Fancy:
        break;
      case AG_Stroked:
        break;
      case AG_Waterfall:
        break;
      }
      break;
    case TextString:
      break;
    }
    emit displayingCountUpdated(displayingCount_);
  }

  painter.end();
}


void
GlyphContinuous::wheelEvent(QWheelEvent* event)
{
  int numSteps = event->angleDelta().y() / 120;
  if (event->modifiers() & Qt::ShiftModifier)
    emit wheelResize(numSteps);
  else if (event->modifiers() == 0)
    emit wheelNavigate(-numSteps);
}


void
GlyphContinuous::paintAGAllGlyphs(QPainter* painter)
{
  for (int i = beginIndex_; i < limitIndex_; i++)
  {
    unsigned index = i;
    if (charMapIndex_ >= 0)
      index = engine_->glyphIndexFromCharCode(i, charMapIndex_);

    if (!paintChar(painter, index))
      break;

    displayingCount_++;
  }
}


void
GlyphContinuous::prePaint()
{
  displayingCount_ = 0;
  engine_->reloadFont();
  metrics_ = engine_->currentFontMetrics();
  x_ = 0;
  // See ftview.c:42
  y_ = ((metrics_.ascender - metrics_.descender + 63) >> 6) + 4;
  stepY_ = ((metrics_.height + 63) >> 6) + 4;
}


bool
GlyphContinuous::paintChar(QPainter* painter,
                           int index)
{
  auto glyph = engine_->loadGlyphWithoutUpdate(index);
  if (!glyph)
    return false;

  // ftview.c:557
  int width = glyph->advance.x ? glyph->advance.x >> 16
                               : metrics_.y_ppem / 2;

  if (!checkFitX(x_ + width))
  {
    x_ = 0;
    y_ += stepY_;

    if (!checkFitY(y_))
      return false;
  }

  x_++; // extra space
  if (glyph->advance.x == 0)
  {
    // Draw a red square to indicate
      painter->fillRect(x_, y_ - width, width, width,
                        Qt::red);
    x_ += width;
  }

  // The real drawing part
  // XXX: this is different from what's being done in
  // `ftcommon.c`:FTDemo_Draw_Slot: is this correct??

  // First translate the outline

  if (glyph->format != FT_GLYPH_FORMAT_OUTLINE)
    return true; // XXX only outline is supported - need to impl others later

  FT_BBox cbox;
  // Don't forget to free this when returning
  auto outline = transformOutlineToOrigin(
                   engine_->ftLibrary(),
                   &reinterpret_cast<FT_OutlineGlyph>(glyph)->outline,
                   &cbox);
  
  auto outlineWidth = (cbox.xMax - cbox.xMin) / 64;
  auto outlineHeight = (cbox.yMax - cbox.yMin) / 64;

  // Then convert to bitmap
  FT_Bitmap bitmap;
  QImage::Format format = QImage::Format_Indexed8;
  auto aaEnabled = engine_->antiAliasingEnabled();

  // TODO cover LCD and color
  if (!aaEnabled)
    format = QImage::Format_Mono;

  // TODO optimization: reuse QImage?
  QImage image(QSize(outlineWidth, outlineHeight), format);

  if (!aaEnabled)
    image.setColorTable(graphicsDefault_->monoColorTable);
  else
    image.setColorTable(graphicsDefault_->grayColorTable);

  image.fill(0);

  bitmap.rows = static_cast<unsigned int>(outlineHeight);
  bitmap.width = static_cast<unsigned int>(outlineWidth);
  bitmap.buffer = image.bits();
  bitmap.pitch = image.bytesPerLine();
  bitmap.pixel_mode = aaEnabled ? FT_PIXEL_MODE_GRAY : FT_PIXEL_MODE_MONO;

  FT_Error error = FT_Outline_Get_Bitmap(engine_->ftLibrary(),
                                         &outline,
                                         &bitmap);
  if (error)
  {
    // XXX error handling
    FT_Outline_Done(engine_->ftLibrary(), &outline);
    return true;
  }

  painter->drawImage(
      QPoint(x_ + cbox.xMin / 64, y_ + (-cbox.yMax / 64)),
      image.convertToFormat(QImage::Format_ARGB32_Premultiplied));

  x_ += width;

  FT_Outline_Done(engine_->ftLibrary(), &outline);
  return true;
}


bool
GlyphContinuous::checkFitX(int x)
{
  return x < width() - 3;
}


bool
GlyphContinuous::checkFitY(int y)
{
  return y < height() - 3;
}


// end of glyphcontinuous.cpp
