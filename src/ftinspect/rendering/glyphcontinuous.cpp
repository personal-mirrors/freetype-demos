// glyphcontinuous.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "glyphcontinuous.hpp"

#include "../engine/engine.hpp"

#include <QPainter>
#include <QWheelEvent>

#include <freetype/ftbitmap.h>


GlyphContinuous::GlyphContinuous(QWidget* parent, Engine* engine)
: QWidget(parent),
  engine_(engine),
  stringRenderer_(engine)
{
  setAcceptDrops(false);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  FT_Stroker_New(engine_->ftLibrary(), &stroker_);
}


GlyphContinuous::~GlyphContinuous()
{
  FT_Stroker_Done(stroker_);
}


void
GlyphContinuous::setSource(Source source)
{
  source_ = source;
  switch (source)
  {
  case SRC_AllGlyphs:
    stringRenderer_.setUseAllGlyphs();
    break;

  case SRC_TextStringRepeated:
  case SRC_TextString:
    updateRendererText();
    break;
  }
}


void
GlyphContinuous::setSourceText(QString text)
{
  text_ = std::move(text);
  updateRendererText();
}


void
GlyphContinuous::paintEvent(QPaintEvent* event)
{
  QPainter painter;
  painter.begin(this);
  painter.fillRect(rect(), Qt::white);

  prePaint();

  paintByRenderer(&painter);
  emit displayingCountUpdated(displayingCount_);

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
GlyphContinuous::paintByRenderer(QPainter* painter)
{
  if (mode_ == M_Stroked)
  {
    auto radius = static_cast<FT_Fixed>(metrics_.y_ppem * 64 * strokeRadius_);
    FT_Stroker_Set(stroker_, radius,
                   FT_STROKER_LINECAP_ROUND,
                   FT_STROKER_LINEJOIN_ROUND,
                   0);
  }

  stringRenderer_.setRepeated(source_ == SRC_TextStringRepeated);
  stringRenderer_.setCallback(
    [&](FT_Glyph glyph)
    {
      drawSingleGlyph(painter, glyph);
    });
  stringRenderer_.setPreprocessCallback(
    [&](FT_Glyph* ptr)
    {
      preprocessGlyph(ptr);
    });
  displayingCount_ = stringRenderer_.render(width(), height(), beginIndex_);
}


void
GlyphContinuous::transformGlyphFancy(FT_Glyph glyph)
{
  // adopted from ftview.c:289
  if (glyph->format == FT_GLYPH_FORMAT_OUTLINE)
  {
    auto outline = reinterpret_cast<FT_OutlineGlyph>(glyph)->outline;
    FT_Glyph_Transform(glyph, &shearMatrix_, NULL);
    if (FT_Outline_EmboldenXY(&outline, emboldeningX_, emboldeningY_))
    {
      // XXX error handling?
      return;
    }

    if (glyph->advance.x)
      glyph->advance.x += emboldeningX_;

    if (glyph->advance.y)
      glyph->advance.y += emboldeningY_;
  }
  else if (glyph->format == FT_GLYPH_FORMAT_BITMAP)
  {
    auto xstr = emboldeningX_ & ~63;
    auto ystr = emboldeningY_ & ~63;

    auto bitmap = &reinterpret_cast<FT_BitmapGlyph>(glyph)->bitmap;
    // No shearing support for bitmap
    FT_Bitmap_Embolden(engine_->ftLibrary(), bitmap, 
                       xstr, ystr);
  }
  else
    return; // XXX no support for SVG
}


FT_Glyph
GlyphContinuous::transformGlyphStroked(FT_Glyph glyph)
{
  // Well, here only outline glyph is supported.
  if (glyph->format != FT_GLYPH_FORMAT_OUTLINE)
    return NULL;
  auto error = FT_Glyph_Stroke(&glyph, stroker_, 0);
  if (error)
    return NULL;
  return glyph;
}


void
GlyphContinuous::prePaint()
{
  displayingCount_ = 0;
  engine_->reloadFont();
  if (engine_->currentFontNumberOfGlyphs() > 0)
    metrics_ = engine_->currentFontMetrics();
  x_ = 0;
  // See ftview.c:42
  y_ = ((metrics_.ascender - metrics_.descender + 63) >> 6) + 4;
  stepY_ = ((metrics_.height + 63) >> 6) + 4;

  // Used by fancy:
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
  

  shearMatrix_.xx = 1 << 16;
  shearMatrix_.xy = static_cast<FT_Fixed>(slant_ * (1 << 16));
  shearMatrix_.yx = 0;
  shearMatrix_.yy = 1 << 16;

  emboldeningX_ = (FT_Pos)(metrics_.y_ppem * 64 * boldX_);
  emboldeningY_ = (FT_Pos)(metrics_.y_ppem * 64 * boldY_);
}


void
GlyphContinuous::updateRendererText()
{
  stringRenderer_.setUseString(text_); // TODO this need to be called when font,
                                       // size or charmap change
}


void
GlyphContinuous::preprocessGlyph(FT_Glyph* glyphPtr)
{
  auto glyph = *glyphPtr;
  switch (mode_)
  {
  case M_Fancy:
    transformGlyphFancy(glyph);
    break;
  case M_Stroked:
  {
    auto stroked = transformGlyphStroked(glyph);
    if (stroked)
    {
      FT_Done_Glyph(glyph);
      *glyphPtr = stroked;
    }
  }
  break;
  default:; // Nothing for M_NORMAL.
  }
}


void
GlyphContinuous::drawSingleGlyph(QPainter* painter, FT_Glyph glyph)
{
  // ftview.c:557
  int width = glyph->advance.x ? glyph->advance.x >> 16
                                : metrics_.y_ppem / 2;
  
  if (glyph->advance.x == 0)
  {
    // Draw a red square to indicate
      painter->fillRect(x_, y_ - width, width, width,
                        Qt::red);
  }

  QRect rect;
  QImage* image = engine_->convertGlyphToQImage(glyph, &rect);
  rect.setTop(height() - rect.top());

  painter->drawImage(rect.topLeft(), *image);
  delete image;
}


// end of glyphcontinuous.cpp
