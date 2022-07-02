// custom_widgets.cpp

// Copyright (C) 2016-2022 by Werner Lemberg.

#include "custom_widgets.hpp"

#include <QStandardItemModel>
#include <QScrollBar>
#include <QStyleOptionButton>

// --------------------------------
// >>>>>>>> QGraphicsViewx <<<<<<<<
// --------------------------------

QGraphicsViewx::QGraphicsViewx(QWidget* parent)
: QGraphicsView(parent), lastBottomLeftPointInitialized_(false)
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

// ------------------------------
// >>>>>>>> QPushButtonx <<<<<<<<
// ------------------------------

// code derived from Qt 4.8.7, function `QPushButton::sizeHint',
// file `src/gui/widgets/qpushbutton.cpp'

QPushButtonx::QPushButtonx(const QString &text,
                           QWidget *parent)
: QPushButton(text, parent)
{
  QStyleOptionButton opt;
  opt.initFrom(this);
  QString s(this->text());
  QFontMetrics fm = fontMetrics();
  QSize sz = fm.size(Qt::TextShowMnemonic, s);
  opt.rect.setSize(sz);

  sz = style()->sizeFromContents(QStyle::CT_PushButton,
                                 &opt,
                                 sz,
                                 this);
  setFixedWidth(sz.width());
}

// ---------------------------
// >>>>>>>> QSpinBoxx <<<<<<<<
// ---------------------------

// we want to mark the center of a pixel square with a single dot or a small
// cross; starting with a certain magnification we thus only use even values
// so that we can do that symmetrically

int
QSpinBoxx::valueFromText(const QString& text) const
{
  int val = QSpinBox::valueFromText(text);

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


QSpinBoxx::QSpinBoxx(QWidget* parent)
: QSpinBox(parent)
{
}


void
QSpinBoxx::stepBy(int steps)
{
  int val = value();

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


// end of custom_widgets.cpp
