// glyphcontinuous.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "glyphcontinuous.hpp"

#include "../engine/engine.hpp"

#include <QPainter>
#include <QWheelEvent>

#include <freetype/ftbitmap.h>


GlyphCacheEntry::~GlyphCacheEntry()
{
  delete image;
}


GlyphCacheEntry::GlyphCacheEntry(GlyphCacheEntry&& other) noexcept
{
  *this = std::move(other);
}


GlyphCacheEntry&
GlyphCacheEntry::operator=(GlyphCacheEntry&& other) noexcept
{
  if (this == &other)
    return *this;

  auto oldImage = image;
  image = other.image;
  basePosition = other.basePosition;
  penPos = other.penPos;
  charCode = other.charCode;
  glyphIndex = other.glyphIndex;
  advance = other.advance;
  other.image = oldImage;
  return *this;
}


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
    positionDelta_ = {};
    break;

  case SRC_TextStringRepeated:
    positionDelta_ = {};
    /* fall through */
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
GlyphContinuous::purgeCache()
{
  glyphCache_.clear();
  currentWritingLine_ = NULL;
}


void
GlyphContinuous::resetPositionDelta()
{
  positionDelta_ = {};
  repaint();
}


void
GlyphContinuous::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.fillRect(rect(), engine_->renderingEngine()->background());

  if (glyphCache_.empty())
    fillCache();
  paintCache(&painter);
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
GlyphContinuous::resizeEvent(QResizeEvent* event)
{
  purgeCache();
  QWidget::resizeEvent(event);
}


void
GlyphContinuous::mousePressEvent(QMouseEvent* event)
{
  QWidget::mousePressEvent(event);
  if (!mouseOperationEnabled_)
    return;
  if (event->button() == Qt::LeftButton)
  {
    prevPositionDelta_ = positionDelta_;
    mouseDownPostition_ = event->pos();
    prevHoriPosition_ = stringRenderer_.position();
    prevIndex_ = beginIndex_;
    // We need to precalculate this value because after the first change of
    // the begin index, the average line count would change. If we don't use the
    // old value, then moving up/down for the same distance would not return
    // to the original index which is confusing.
    averageLineCount_ = calculateAverageLineCount();
  }
}


void
GlyphContinuous::mouseMoveEvent(QMouseEvent* event)
{
  QWidget::mouseMoveEvent(event);
  if (!mouseOperationEnabled_)
    return;
  if (event->buttons() != Qt::LeftButton)
    return;
  auto delta = event->pos() - mouseDownPostition_;
  if (source_ == SRC_AllGlyphs)
  {
    auto deltaIndex = -delta.x() / HorizontalUnitLength
                          - delta.y() / VerticalUnitLength * averageLineCount_;
    if (prevIndex_ + deltaIndex != beginIndex_)
      emit beginIndexChangeRequest(beginIndex_ + deltaIndex);
  }
  else if (source_ == SRC_TextString)
  {
    positionDelta_ = prevPositionDelta_ + delta;
    positionDelta_.setX(0); // Don't move horizontally

    // but use the renderer
    auto horiPos = delta.x() / static_cast<double>(width());
    horiPos += prevHoriPosition_;
    horiPos = qBound(0.0, horiPos, 1.0);
    stringRenderer_.setPosition(horiPos);

    purgeCache();
    repaint();
  }
}


void
GlyphContinuous::mouseReleaseEvent(QMouseEvent* event)
{
  QWidget::mouseReleaseEvent(event);
  if (!mouseOperationEnabled_)
    return;
  if (event->button() == Qt::LeftButton)
  {
    auto dist = event->pos() - mouseDownPostition_;
    if (dist.manhattanLength() < ClickDragThreshold)
    {
      auto gl = findGlyphByMouse(event->pos(), NULL);
      if (gl)
        emit updateGlyphDetails(gl, stringRenderer_.charMapIndex(), true);
    }
  }
  else if (event->button() == Qt::RightButton)
  {
    double size;
    auto gl = findGlyphByMouse(event->pos(), &size);
    if (gl)
      emit rightClickGlyph(gl->glyphIndex, size);
  }
}


void
GlyphContinuous::paintByRenderer()
{
  purgeCache();

  stringRenderer_.setRepeated(source_ == SRC_TextStringRepeated);
  stringRenderer_.setCallback(
    [&](FT_Glyph glyph, FT_Vector penPos, GlyphContext& ctx)
    {
      saveSingleGlyph(glyph, penPos, ctx);
    });
  stringRenderer_.setImageCallback(
    [&](QImage* image,
        QRect pos, 
        FT_Vector penPos, FT_Vector advance,
        GlyphContext& ctx)
    {
      saveSingleGlyphImage(image, pos, penPos, advance, ctx);
    });
  stringRenderer_.setPreprocessCallback(
    [&](FT_Glyph* ptr)
    {
      preprocessGlyph(ptr);
    });
  stringRenderer_.setLineBeginCallback(
    [&](FT_Vector pos, double size)
    {
      beginSaveLine(pos, size);
    });
  displayingCount_ = stringRenderer_.render(width(), height(), beginIndex_);
}


void
GlyphContinuous::transformGlyphFancy(FT_Glyph glyph)
{
  auto& metrics = engine_->currentFontMetrics();
  auto emboldeningX = (FT_Pos)(metrics.y_ppem * 64 * boldX_);
  auto emboldeningY = (FT_Pos)(metrics.y_ppem * 64 * boldY_);
  // adopted from ftview.c:289
  if (glyph->format == FT_GLYPH_FORMAT_OUTLINE)
  {
    auto outline = reinterpret_cast<FT_OutlineGlyph>(glyph)->outline;
    FT_Glyph_Transform(glyph, &shearMatrix_, NULL);
    if (FT_Outline_EmboldenXY(&outline, emboldeningX, emboldeningY))
    {
      // XXX error handling?
      return;
    }

    if (glyph->advance.x)
      glyph->advance.x += emboldeningX;

    if (glyph->advance.y)
      glyph->advance.y += emboldeningY;
  }
  else if (glyph->format == FT_GLYPH_FORMAT_BITMAP)
  {
    auto xstr = emboldeningX & ~63;
    auto ystr = emboldeningY & ~63;

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
GlyphContinuous::paintCache(QPainter* painter)
{
  if (stringRenderer_.isWaterfall())
    positionDelta_.setY(0);
  for (auto& line : glyphCache_)
  {
    beginDrawCacheLine(painter, line);
    for (auto& glyph : line.entries)
    {
      drawCacheGlyph(painter, glyph);
    }
  }
}


void
GlyphContinuous::fillCache()
{
  prePaint();
  paintByRenderer();
  emit displayingCountUpdated(displayingCount_);
}


void
GlyphContinuous::prePaint()
{
  displayingCount_ = 0;

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
}


void
GlyphContinuous::updateStroke()
{
  if (mode_ != M_Stroked || !engine_->renderReady())
    return;

  auto& metrics = engine_->currentFontMetrics();
  auto radius = static_cast<FT_Fixed>(metrics.y_ppem * 64 * strokeRadius_);
  strokeRadiusForSize_ = radius;
  FT_Stroker_Set(stroker_, radius,
                 FT_STROKER_LINECAP_ROUND,
                 FT_STROKER_LINEJOIN_ROUND,
                 0);
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
GlyphContinuous::beginSaveLine(FT_Vector pos,
                               double sizePoint)
{
  glyphCache_.emplace_back();
  currentWritingLine_ = &glyphCache_.back();
  currentWritingLine_->sizePoint = sizePoint;
  currentWritingLine_->basePosition = { static_cast<int>(pos.x),
                                        static_cast<int>(pos.y) };
}


void
GlyphContinuous::saveSingleGlyph(FT_Glyph glyph,
                                 FT_Vector penPos,
                                 GlyphContext gctx)
{
  if (!currentWritingLine_)
    return;

  QRect rect;
  QImage* image = engine_->renderingEngine()->convertGlyphToQImage(glyph, 
                                                                   &rect, 
                                                                   true);
  saveSingleGlyphImage(image, rect, penPos, glyph->advance, gctx);
}


void
GlyphContinuous::saveSingleGlyphImage(QImage* image,
                                      QRect rect,
                                      FT_Vector penPos,
                                      FT_Vector advance,
                                      GlyphContext gctx)
{
  if (!currentWritingLine_)
    return;

  currentWritingLine_->entries.emplace_back();
  auto& entry = currentWritingLine_->entries.back();

  QPoint penPosPoint = { static_cast<int>(penPos.x),
                         static_cast<int>(penPos.y) };

  rect.translate(penPosPoint);

  entry.image = image;
  entry.basePosition = rect;
  entry.charCode = gctx.charCode;
  entry.glyphIndex = gctx.glyphIndex;
  entry.advance = advance;
  entry.penPos = penPosPoint;
  entry.yPpem = engine_->currentFontMetrics().y_ppem;
}


void
GlyphContinuous::beginDrawCacheLine(QPainter* painter,
                                    GlyphCacheLine& line)
{
  // Now only used by waterfall mode to draw a size indicator.
  if (!stringRenderer_.isWaterfall())
  {
    sizeIndicatorOffset_ = 0;
    return;
  }

  auto oldFont = painter->font();
  oldFont.setPointSizeF(line.sizePoint);
  painter->setFont(oldFont);
  auto metrics = painter->fontMetrics();

  auto printSize = line.sizePoint;
  if (engine_->currentFontBitmapOnly())
    printSize = printSize * engine_->dpi() / 72.0; // convert back
  auto sizePrefix = QString("%1: ").arg(printSize);
  painter->drawText(line.basePosition, sizePrefix);

  sizeIndicatorOffset_ = metrics.horizontalAdvance(sizePrefix);
  line.sizeIndicatorOffset = sizeIndicatorOffset_;
}


void
GlyphContinuous::drawCacheGlyph(QPainter* painter,
                                const GlyphCacheEntry& entry)
{
  // ftview.c:557
  // Well, metrics is also part of the cache...
  int width = entry.advance.x ? entry.advance.x >> 16
                              : entry.yPpem / 2;

  if (entry.advance.x == 0 && !stringRenderer_.isWaterfall())
  {
    // Draw a red square to indicate
    auto squarePoint = entry.penPos;
    squarePoint.setY(squarePoint.y() - width);
    painter->fillRect(QRect(squarePoint, QSize(width, width)), Qt::red);
  }

  QRect rect = entry.basePosition;
  rect.moveLeft(rect.x() + sizeIndicatorOffset_);
  rect.translate(positionDelta_);

  painter->drawImage(rect.topLeft(), *entry.image);
}


GlyphCacheEntry*
GlyphContinuous::findGlyphByMouse(QPoint position,
                                  double* outSizePoint)
{
  position -= positionDelta_;
  for (auto& line : glyphCache_)
    for (auto& entry : line.entries)
    {
      auto rect = entry.basePosition;
      rect.moveLeft(rect.x() + line.sizeIndicatorOffset);
      if (rect.contains(position))
      {
        if (outSizePoint)
          *outSizePoint = line.sizePoint;
        return &entry;
      }
    }
  return NULL;
}


int
GlyphContinuous::calculateAverageLineCount()
{
  int averageLineCount = 0;
  for (auto& line : glyphCache_)
  {
    // line.entries.size must < INT_MAX because the total glyph count in
    // the renderer is below that
    averageLineCount += static_cast<int>(line.entries.size());
  }
  if (!glyphCache_.empty())
    averageLineCount /= static_cast<int>(glyphCache_.size());
  return averageLineCount;
}


// end of glyphcontinuous.cpp