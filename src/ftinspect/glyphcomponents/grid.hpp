// grid.hpp

// Copyright (C) 2016-2023 by
// Werner Lemberg.


#pragma once

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QPen>


class Grid
: public QGraphicsItem
{
public:
  Grid(QGraphicsView* parentView);
  QRectF boundingRect() const override;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

  void setShowGrid(bool showGrid,
                   bool showAuxLines);
  void updateParameters(int ascenderPx,
                        int descenderPx,
                        int advancePx);

  void updateRect(); // There's no signal/slots for QGraphicsItem.

private:
  QGraphicsView* parentView_;
  QRectF rect_;
  QRectF sceneRect_;

  bool showGrid_ = true;
  bool showAuxLines_ = false;

  int ascender_ = 0;
  int descender_ = 0;
  int advance_ = 0;
};


// end of grid.hpp
