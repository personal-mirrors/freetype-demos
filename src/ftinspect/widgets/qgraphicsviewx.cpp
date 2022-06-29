// qgraphicsviewx.hpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#include "qgraphicsviewx.hpp"

#include <QScrollBar>


QGraphicsViewx::QGraphicsViewx()
: lastBottomLeftPointInitialized_(false)
{
  // empty
}


void
QGraphicsViewx::scrollContentsBy(int dx,
                                 int dy)
{
  QGraphicsView::scrollContentsBy(dx, dy);
  lastBottomLeftPoint_ = viewport()->rect().bottomLeft();
}


void
QGraphicsViewx::resizeEvent(QResizeEvent* event)
{
  QGraphicsView::resizeEvent(event);

  // XXX I don't know how to properly initialize this value,
  //     thus the hack with the boolean
  if (!lastBottomLeftPointInitialized_)
  {
    lastBottomLeftPoint_ = viewport()->rect().bottomLeft();
    lastBottomLeftPointInitialized_ = true;
  }

  QPointF currentBottomLeftPoint = viewport()->rect().bottomLeft();
  int verticalPosition = verticalScrollBar()->value();
  verticalScrollBar()->setValue(static_cast<int>(
                                  verticalPosition
                                  - (currentBottomLeftPoint.y()
                                     - lastBottomLeftPoint_.y())));
}


// end of qgraphicsviewx.cpp
