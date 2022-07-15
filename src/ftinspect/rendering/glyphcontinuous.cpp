// glyphcontinuous.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "glyphcontinuous.hpp"

#include "../engine/engine.hpp"
#include "../rendering/renderutils.hpp"

#include <cmath>
#include <QPainter>
#include <QWheelEvent>

#include <freetype/ftbitmap.h>


GlyphContinuous::GlyphContinuous(QWidget* parent, Engine* engine)
: QWidget(parent), engine_(engine)
{
  setAcceptDrops(false);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  FT_Stroker_New(engine_->ftLibrary(), &stroker_);
}


GlyphContinuous::~GlyphContinuous()
{
  cleanCloned();
  FT_Stroker_Done(stroker_);
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

    switch (source_)
    {
    case SRC_AllGlyphs:
      switch (mode_)
      {
      case M_Normal:
      case M_Fancy:
      case M_Stroked:
        paintAG(&painter);
        break;
      }
      break;
    case SRC_TextString:
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
  if (mode_ == M_Stroked)
  {
    auto radius = static_cast<FT_Fixed>(metrics_.y_ppem * 64 * strokeRadius_);
    FT_Stroker_Set(stroker_, radius,
                   FT_STROKER_LINECAP_ROUND,
                   FT_STROKER_LINEJOIN_ROUND,
                   0);
  }

  for (int i = beginIndex_; i < limitIndex_; i++)
  {
    unsigned index = i;
    if (charMapIndex_ >= 0)
      index = engine_->glyphIndexFromCharCode(i, charMapIndex_);

    if (!loadGlyph(index))
      break;

    // All Glyphs need no tranformation, and Waterfall isn't handled here.
    switch (mode_)
    {
    case M_Fancy:
      transformGlyphFancy();
      break;
    case M_Stroked:
      transformGlyphStroked();
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
GlyphContinuous::transformGlyphFancy()
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
  shear.xy = static_cast<FT_Fixed>(slant_ * (1 << 16));
  shear.yx = 0;
  shear.yy = 1 << 16;

  xstr = (FT_Pos)(metrics_.y_ppem * 64 * boldX_);
  ystr = (FT_Pos)(metrics_.y_ppem * 64 * boldY_);

  if (glyph_->format == FT_GLYPH_FORMAT_OUTLINE)
  {
    if (!isGlyphCloned_)
      cloneGlyph();
    FT_Outline_Transform(&outline_, &shear);
    if (FT_Outline_EmboldenXY(&outline_, xstr, ystr))
    {
      // XXX error handling?
      return;
    }

    if (glyph_->advance.x)
      glyph_->advance.x += xstr;

    if (glyph_->advance.y)
      glyph_->advance.y += ystr;
  }
  else if (glyph_->format == FT_GLYPH_FORMAT_BITMAP)
  {
    if (!isBitmapCloned_)
      cloneBitmap();
    xstr &= ~63;
    ystr &= ~63;

    // No shearing support for bitmap
    FT_Bitmap_Embolden(engine_->ftLibrary(), &bitmap_, 
                       xstr, ystr);
  }
  else
    return; // XXX no support for SVG
}


void
GlyphContinuous::transformGlyphStroked()
{
  // Well, here only outline glyph is supported.
  if (glyph_->format != FT_GLYPH_FORMAT_OUTLINE)
    return;
  auto oldGlyph = glyph_;
  auto error = FT_Glyph_Stroke(&glyph_, stroker_, 0);
  if (!error)
  {
    if (isGlyphCloned_)
      FT_Done_Glyph(oldGlyph);
    isGlyphCloned_ = true;
    outline_ = reinterpret_cast<FT_OutlineGlyph>(glyph_)->outline;
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

  QImage* image;
  
  if (bitmap_.buffer) // Always prefer `bitmap_` since it can be manipulated
    image = engine_->convertBitmapToQImage(&bitmap_);
  else
    image = engine_->convertGlyphToQImage(glyph_, NULL);
  auto offset = engine_->computeGlyphOffset(glyph_);

  painter->drawImage(offset + QPoint(x_, y_),
                     *image);
  delete image;

  x_ += width;
  return true;
}


bool
GlyphContinuous::loadGlyph(int index)
{
  if (isGlyphCloned_)
    FT_Done_Glyph(glyph_);
  glyph_ = engine_->loadGlyphWithoutUpdate(index);
  isGlyphCloned_ = false;
  if (!glyph_)
    return false;
  refreshOutlineOrBitmapFromGlyph();
  return true;
}


void
GlyphContinuous::cloneGlyph()
{
  if (isGlyphCloned_)
    return;
  glyph_ = ::cloneGlyph(glyph_);
  refreshOutlineOrBitmapFromGlyph();
  isGlyphCloned_ = true;
}


void
GlyphContinuous::cloneBitmap()
{
  if (isBitmapCloned_)
    return;
  bitmap_ = ::cloneBitmap(engine_->ftLibrary(), &bitmap_);
  isBitmapCloned_ = true;
}


void
GlyphContinuous::refreshOutlineOrBitmapFromGlyph()
{
  if (glyph_->format == FT_GLYPH_FORMAT_OUTLINE)
  {
    outline_ = reinterpret_cast<FT_OutlineGlyph>(glyph_)->outline;

    // Make `bitmap_` invalid
    if (isBitmapCloned_)
      FT_Bitmap_Done(engine_->ftLibrary(), &bitmap_);
    isBitmapCloned_ = false;
    bitmap_.buffer = NULL;
  }
  else if (glyph_->format == FT_GLYPH_FORMAT_BITMAP)
  {
    // Initialize `bitmap_`
    if (isBitmapCloned_)
      FT_Bitmap_Done(engine_->ftLibrary(), &bitmap_);
    isBitmapCloned_ = false;
    bitmap_ = reinterpret_cast<FT_BitmapGlyph>(glyph_)->bitmap;

    outline_.points = NULL;
  }
  else
  {
    // Both invalid.
    outline_.points = NULL;

    if (isBitmapCloned_)
      FT_Bitmap_Done(engine_->ftLibrary(), &bitmap_);
    isBitmapCloned_ = false;
    bitmap_.buffer = NULL;
  }
}


void
GlyphContinuous::cleanCloned()
{
  if (isGlyphCloned_)
  {
    if (glyph_->format == FT_GLYPH_FORMAT_BITMAP && !isBitmapCloned_)
      bitmap_.buffer = NULL;

    FT_Done_Glyph(glyph_);
    isGlyphCloned_ = false;
  }
  if (isBitmapCloned_)
  {
    FT_Bitmap_Done(engine_->ftLibrary(), &bitmap_);
    bitmap_.buffer = NULL;
    isBitmapCloned_ = false;
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
