// glyphbitmap.hpp

// Copyright (C) 2016-2023 by
// Werner Lemberg.


#pragma once

#include <QGraphicsItem>
#include <QPaintEvent>
#include <QPen>
#include <QWidget>

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>


class Engine;

class GlyphBitmap
: public QGraphicsItem
{
public:
  GlyphBitmap(QImage* image,
              QRect rect);
  GlyphBitmap(int glyphIndex,
              FT_Glyph glyph,
              Engine* engine);
  ~GlyphBitmap() override;
  QRectF boundingRect() const override;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

private:
  QImage* image_ = NULL;
  QRectF boundingRect_;
};


// Sometimes we don't want a complicated `QGraphicsView`
// for this kind of work...
class GlyphBitmapWidget
: public QWidget
{
  Q_OBJECT
public:
  GlyphBitmapWidget(QWidget* parent);
  ~GlyphBitmapWidget() override;

  void updateImage(QImage* image,
                   QRect rect,
                   QRect placeholderRect = {});
  void releaseImage();

signals:
  void clicked();

protected:
  void paintEvent(QPaintEvent* event) override;
  QSize sizeHint() const override;
  void mouseReleaseEvent(QMouseEvent* event) override;

private:
  GlyphBitmap* bitmapItem_ = NULL;
  QRect rect_ = {};
  QRect placeholderRect_ = {};
};


// end of glyphbitmap.hpp
