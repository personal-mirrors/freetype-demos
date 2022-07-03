// grid.cpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#include "grid.hpp"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsWidget>
#include <QGraphicsView>


Grid::Grid(QGraphicsView* parentView, 
           const QPen& gridP,
           const QPen& axisP)
: gridPen_(gridP),
  axisPen_(axisP),
  parentView_(parentView)
{
 // empty
  updateRect();
}


QRectF
Grid::boundingRect() const
{
  return rect_;
}


void
Grid::updateRect()
{
  auto viewport = parentView_->mapToScene(parentView_->viewport()->geometry())
                         .boundingRect()
                         .toRect();
  int minX = std::min(viewport.left() - 10, -100);
  int minY = std::min(viewport.top() - 10, -100);
  int maxX = std::max(viewport.right() + 10, 100);
  int maxY = std::max(viewport.bottom() + 10, 100);

  auto newSceneRect = QRectF(QPointF(minX - 20, minY - 20), 
                             QPointF(maxX + 20, maxY + 20));
  if (sceneRect_ != newSceneRect && scene())
  {
    scene()->setSceneRect(newSceneRect);
    sceneRect_ = newSceneRect;
  }

  // no need to take care of pen width
  rect_ = QRectF(QPointF(minX, minY), 
                QPointF(maxX, maxY));
}


// XXX call this in a `myQDraphicsView::drawBackground' derived method
//     to always fill the complete viewport

void
Grid::paint(QPainter* painter,
            const QStyleOptionGraphicsItem* option,
            QWidget* widget)
{
  auto br = boundingRect().toRect();
  int minX = br.left();
  int minY = br.top();
  int maxX = br.right();
  int maxY = br.bottom();

  const qreal lod = option->levelOfDetailFromTransform(
                              painter->worldTransform());

  painter->setPen(gridPen_);

  // don't mark pixel center with a cross if magnification is too small
  if (lod > 20)
  {
    int halfLength = 1;

    // cf. QSpinBoxx
    if (lod > 640)
      halfLength = 6;
    else if (lod > 320)
      halfLength = 5;
    else if (lod > 160)
      halfLength = 4;
    else if (lod > 80)
      halfLength = 3;
    else if (lod > 40)
      halfLength = 2;

    for (qreal x = minX; x < maxX; x++)
      for (qreal y = minY; y < maxY; y++)
      {
        painter->drawLine(QLineF(x + 0.5, y + 0.5 - halfLength / lod,
                                 x + 0.5, y + 0.5 + halfLength / lod));
        painter->drawLine(QLineF(x + 0.5 - halfLength / lod, y + 0.5,
                                 x + 0.5 + halfLength / lod, y + 0.5));
      }
  }

  // don't draw grid if magnification is too small
  if (lod >= 5)
  {
    for (int x = minX; x <= maxX; x++)
      painter->drawLine(x, minY,
                        x, maxY);
    for (int y = minY; y <= maxY; y++)
      painter->drawLine(minX, y,
                        maxX, y);
  }

  painter->setPen(axisPen_);

  painter->drawLine(0, minY,
                    0, maxY);
  painter->drawLine(minX, 0,
                    maxX, 0);
}


// end of grid.cpp
