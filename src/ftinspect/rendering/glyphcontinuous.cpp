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
      case AG_Fancy:
      case AG_Stroked:
        paintAG(&painter);
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
GlyphContinuous::paintAG(QPainter* painter)
{
  for (int i = beginIndex_; i < limitIndex_; i++)
  {
    unsigned index = i;
    if (charMapIndex_ >= 0)
      index = engine_->glyphIndexFromCharCode(i, charMapIndex_);

    if (!loadGlyph(index))
      break;

    // All Glyphs need no tranformation, and Waterfall isn't handled here.
    switch (modeAG_)
    {
    case AG_Fancy:
      transformGlyphAGFancy();
      break;
    case AG_Stroked:
      transformGlyphAGStroked();
      break;
    default:;
    }

    if (!paintChar(painter))
      break;
    cleanCloned();

    displayingCount_++;
  }
  cleanCloned();
}


void
GlyphContinuous::transformGlyphAGFancy()
{
  // adopted from ftview.c:289
  /***************************************************************/
  /*                                                             */
  /*  2*2 affine transformation matrix, 16.16 fixed float format */
  /*                                                             */
  /*  Shear matrix:                                              */
  /*                                                             */
  /*         | x' |     | 1  k |   | x |          x' = x + ky    */
  /*         |    |  =  |      | * |   |   <==>                  */
  /*         | y' |     | 0  1 |   | y |          y' = y         */
  /*                                                             */
  /*        outline'     shear    outline                        */
  /*                                                             */
  /***************************************************************/

  FT_Matrix shear;
  FT_Pos xstr, ystr;

  shear.xx = 1 << 16;
  shear.xy = (FT_Fixed)(slant_ * (1 << 16));
  shear.yx = 0;
  shear.yy = 1 << 16;

  xstr = (FT_Pos)(metrics_.y_ppem * 64 * boldX_);
  ystr = (FT_Pos)(metrics_.y_ppem * 64 * boldY_);

  if (!isGlyphCloned_)
    cloneGlyph();

  if (glyph_->format != FT_GLYPH_FORMAT_OUTLINE)
    return; // TODO suuport non-outline: code below all depend on `outline_`!

  FT_Outline_Transform(&outline_, &shear);
  FT_Outline_EmboldenXY(&outline_, xstr, ystr);

  if (glyph_->advance.x)
    glyph_->advance.x += xstr;

  if (glyph_->advance.y)
    glyph_->advance.y += ystr;
  
  //glyph_->metrics.width += xstr;
  //glyph_->metrics.height += ystr;
  //glyph_->metrics.horiAdvance += xstr;
  //glyph_->metrics.vertAdvance += ystr;
}


void
GlyphContinuous::transformGlyphAGStroked()
{
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
GlyphContinuous::paintChar(QPainter* painter)
{
  // ftview.c:557
  int width = glyph_->advance.x ? glyph_->advance.x >> 16
                                : metrics_.y_ppem / 2;

  if (!checkFitX(x_ + width))
  {
    x_ = 0;
    y_ += stepY_;

    if (!checkFitY(y_))
      return false;
  }

  x_++; // extra space
  if (glyph_->advance.x == 0)
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

  if (glyph_->format != FT_GLYPH_FORMAT_OUTLINE)
    return true; // XXX only outline is supported - need to impl others later

  FT_BBox cbox;
  // Don't forget to free this when returning
  if (!isOutlineCloned_ && !isGlyphCloned_)
    cloneOutline();
  
  transformOutlineToOrigin(&outline_, &cbox);
  
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
                                         &outline_,
                                         &bitmap);
  if (error)
  {
    // XXX error handling
    return true;
  }

  painter->drawImage(
      QPoint(x_ + cbox.xMin / 64, y_ + (-cbox.yMax / 64)),
      image.convertToFormat(QImage::Format_ARGB32_Premultiplied));

  x_ += width;
  
  return true;
}


bool
GlyphContinuous::loadGlyph(int index)
{
  glyph_ = engine_->loadGlyphWithoutUpdate(index);
  isGlyphCloned_ = false;
  if (!glyph_)
    return false;
  if (glyph_->format == FT_GLYPH_FORMAT_OUTLINE)
  {
    isOutlineCloned_ = false;
    outline_ = reinterpret_cast<FT_OutlineGlyph>(glyph_)->outline;
  }
  return true;
}


void
GlyphContinuous::cloneGlyph()
{
  glyph_ = ::cloneGlyph(glyph_);
  isGlyphCloned_ = true;
}


void
GlyphContinuous::cloneOutline()
{
  outline_ = ::cloneOutline(engine_->ftLibrary(), &outline_);
  isOutlineCloned_ = true;
}


void
GlyphContinuous::cleanCloned()
{
  if (isGlyphCloned_)
  {
    FT_Done_Glyph(glyph_);
    isGlyphCloned_ = false;
  }
  if (isOutlineCloned_)
  {
    FT_Outline_Done(engine_->ftLibrary(), &outline_);
    isOutlineCloned_ = false;
  }
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
