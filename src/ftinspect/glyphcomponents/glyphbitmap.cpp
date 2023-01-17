// glyphbitmap.cpp

// Copyright (C) 2016-2023 by
// Werner Lemberg.


#include "../engine/engine.hpp"
#include "glyphbitmap.hpp"

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
  image_ = engine->renderingEngine()->tryDirectRenderColorLayers(
                                        glyphIndex,
                                        &bRect,
                                        true);

  if (!image_)
    image_ = engine->renderingEngine()->convertGlyphToQImage(glyph,
                                                             &bRect,
                                                             true);
  boundingRect_ = bRect; // QRect to QRectF.
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
  // deviate from the grid lines.
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


GlyphBitmapWidget::GlyphBitmapWidget(QWidget* parent)
: QWidget(parent)
{
  setToolTip(tr("Click to inspect in Singular Grid View."));
}


GlyphBitmapWidget::~GlyphBitmapWidget()
{
  delete bitmapItem_;
  bitmapItem_ = NULL;
}


void
GlyphBitmapWidget::updateImage(QImage* image,
                               QRect rect,
                               QRect placeholderRect)
{
  delete bitmapItem_;
  auto* copied = new QImage(image->copy());

  rect_ = rect;
  placeholderRect_ = placeholderRect;
  auto zeroedRect = rect; // `GlyphBitmap` doesn't play well with offset.
  zeroedRect.moveTopLeft({ 0, 0 });
  bitmapItem_ = new GlyphBitmap(copied, zeroedRect);

  repaint();
}


void
GlyphBitmapWidget::releaseImage()
{
  delete bitmapItem_;
  bitmapItem_ = NULL;
  repaint();
}


void
GlyphBitmapWidget::paintEvent(QPaintEvent* event)
{
  if (!bitmapItem_)
    return;
  auto s = size();

  auto br = QRect(QPoint(std::min(rect_.left(), placeholderRect_.left()),
                         std::min(rect_.top(), placeholderRect_.top())),
                  QPoint(std::max(rect_.right(), placeholderRect_.right()),
                         std::max(rect_.bottom(), placeholderRect_.bottom())));

  double xScale = 0.9 * s.width() / br.width();
  double yScale = 0.9 * s.height() / br.height();
  auto margin = br.width() * 0.05;
  auto scale = std::min(xScale, yScale);

  QPainter painter(this);
  painter.fillRect(rect(), Qt::white);
  painter.save(); // Push before scaling.
  painter.scale(scale, scale);
  painter.translate(-br.topLeft() + QPointF(margin, margin));

  double scaledLineWidth = 4 / scale;
  double scaledLineWidthHalf = scaledLineWidth / 2;
  painter.setPen(QPen(Qt::blue, scaledLineWidth));
  // Blue line: Ink box.
  painter.drawRect(QRectF(rect_).adjusted(-scaledLineWidthHalf,
                                          -scaledLineWidthHalf,
                                          scaledLineWidthHalf,
                                          scaledLineWidthHalf));

  painter.save(); // Push before translating.
  painter.translate(rect_.topLeft());

  QStyleOptionGraphicsItem ogi;
  ogi.exposedRect = br;
  bitmapItem_->paint(&painter, &ogi, this);

  painter.restore(); // Undo translation.

  scaledLineWidth /= 2;
  scaledLineWidthHalf /= 2;
  // Light gray line: EM box.
  painter.setPen(QPen(Qt::lightGray, scaledLineWidth));
  painter.drawRect(QRectF(placeholderRect_).adjusted(-scaledLineWidthHalf,
                                                     -scaledLineWidthHalf,
                                                     scaledLineWidthHalf,
                                                     scaledLineWidthHalf));

  auto tfForAxis = painter.transform().inverted();
  painter.setPen(QPen(Qt::black, scaledLineWidth / 2));
  // Thin black line: x- and y-axis.
  painter.drawLine(QPointF(tfForAxis.map(QPointF(0, 0)).x(), 0),
                   QPointF(tfForAxis.map(QPointF(s.width(), 0)).x(), 0));
  painter.drawLine(QPointF(0, tfForAxis.map(QPointF(0, 0)).y()),
                   QPointF(0, tfForAxis.map(QPointF(0, s.height())).y()));

  painter.restore(); // Undo scaling.

  // Main border.
  painter.setPen(QPen(Qt::black, 4));
  painter.drawRect(rect().adjusted(2, 2, -2, -2));
}


QSize
GlyphBitmapWidget::sizeHint() const
{
  return { 300, 300 };
}


void
GlyphBitmapWidget::mouseReleaseEvent(QMouseEvent* event)
{
  QWidget::mouseReleaseEvent(event);
  if (event->button() == Qt::LeftButton)
    emit clicked();
}


// end of glyphbitmap.cpp
