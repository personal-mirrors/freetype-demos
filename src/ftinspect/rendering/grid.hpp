// grid.hpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#pragma once

#include <QGraphicsItem>
#include <QPen>

class Grid
: public QGraphicsItem
{
public:
  Grid(QGraphicsView* parentView,
       const QPen& gridPen,
       const QPen& axisPen);
  QRectF boundingRect() const override;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

  void updateRect(); // there's no signal/slots for QGraphicsItem.

private:
  QPen gridPen_;
  QPen axisPen_;

  QGraphicsView* parentView_;
  QRectF rect_;
  QRectF sceneRect_;
};


// end of grid.hpp
