// customwidgets.cpp

// Copyright (C) 2016-2023 by
// Werner Lemberg.

#include "customwidgets.hpp"

#include <qevent.h>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QStyleOptionButton>


// --------------------------------
// >>>>>>>> QGraphicsViewx <<<<<<<<
// --------------------------------

QGraphicsViewx::QGraphicsViewx(QWidget* parent)
: QGraphicsView(parent),
  lastBottomLeftPointInitialized_(false)
{
  // empty
}


void
QGraphicsViewx::wheelEvent(QWheelEvent* event)
{
  if (event->modifiers() & Qt::ShiftModifier)
    emit shiftWheelEvent(event);
  else if (event->modifiers() & Qt::ControlModifier)
    emit ctrlWheelEvent(event);
  else
    QGraphicsView::wheelEvent(event);
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


// ---------------------------
// >>>>>>>> ZoomSpinBox <<<<<<<<
// ---------------------------

// We want to mark the center of a pixel square with a single dot or a small
// cross; starting with a certain magnification we thus only use even values
// so that we can do that symmetrically.
// This behaviour is only for the singular view grid.

ZoomSpinBox::ZoomSpinBox(QWidget* parent,
                         bool continuousView)
: QDoubleSpinBox(parent),
  continuousView_(continuousView)
{
  setKeyboardTracking(false);
  if (continuousView)
  {
    setDecimals(2);
    setRange(0.25, 50.0);
    setSingleStep(0.25);
    setValue(1);
  }
  else
  {
    setDecimals(0);
    setRange(1, 1000 - 1000 % 64);
    setSingleStep(1);
    setValue(20);
  }
}


double
ZoomSpinBox::valueFromText(const QString& text) const
{
  if (continuousView_)
    return QDoubleSpinBox::valueFromText(text);
  int val = static_cast<int>(QDoubleSpinBox::valueFromText(text));
  if (val > 640)
    val = val - (val % 64);
  else if (val > 320)
    val = val - (val % 32);
  else if (val > 160)
    val = val - (val % 16);
  else if (val > 80)
    val = val - (val % 8);
  else if (val > 40)
    val = val - (val % 4);
  else if (val > 20)
    val = val - (val % 2);

  return val;
}


void
ZoomSpinBox::stepBy(int steps)
{
  if (continuousView_)
  {
    QDoubleSpinBox::stepBy(steps);
    return;
  }

  int val = static_cast<int>(value());

  if (steps > 0)
  {
    for (int i = 0; i < steps; i++)
    {
      if (val >= 640)
        val = val + 64;
      else if (val >= 320)
        val = val + 32;
      else if (val >= 160)
        val = val + 16;
      else if (val >= 80)
        val = val + 8;
      else if (val >= 40)
        val = val + 4;
      else if (val >= 20)
        val = val + 2;
      else
        val++;
    }
  }
  else if (steps < 0)
  {
    for (int i = 0; i < -steps; i++)
    {
      if (val > 640)
        val = val - 64;
      else if (val > 320)
        val = val - 32;
      else if (val > 160)
        val = val - 16;
      else if (val > 80)
        val = val - 8;
      else if (val > 40)
        val = val - 4;
      else if (val > 20)
        val = val - 2;
      else
        val--;
    }
  }

  setValue(val);
}


UnboundScrollArea::UnboundScrollArea(QWidget* parent)
: QScrollArea(parent)
{
}


QSize
UnboundScrollArea::sizeHint() const
{
  int fw = 2 * frameWidth();
  QSize sz(fw, fw);

  int h = fontMetrics().height();

  auto w = widget();
  if (w)
    sz += widgetResizable() ? w->sizeHint() : w->size();
  else
    sz += QSize(12 * h, 8 * h);

  if (verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOn)
    sz.setWidth(sz.width() + verticalScrollBar()->sizeHint().width());
  if (horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOn)
    sz.setHeight(sz.height() + horizontalScrollBar()->sizeHint().height());
  return sz;
}


// end of customwidgets.cpp
