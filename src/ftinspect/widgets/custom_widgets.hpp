// custom_widgets.hpp

// Copyright (C) 2016-2022 by Werner Lemberg.

#pragma once

#include <QComboBox>
#include <QGraphicsView>
#include <QPushButton>
#include <QSpinBox>
#include <QString>

// We need to define a series of custom Qt widgets to satisfy.
// Basically those custom widgets are derived classes from Qt-provided components,
// with minor changes.
// Because all those derived classes are pretty tiny and not core logic, they're
// organized into one single hpp/cpp pair.

// we want to grey out items in a combo box;
// since Qt doesn't provide a function for this we derive a class
class QComboBoxx
: public QComboBox
{
  Q_OBJECT

public:
  void setItemEnabled(int index,
                      bool enable);
};


// we want to anchor the view at the bottom left corner
// while the windows gets resized
class QGraphicsViewx
: public QGraphicsView
{
  Q_OBJECT

public:
  QGraphicsViewx();

protected:
  void resizeEvent(QResizeEvent* event);
  void scrollContentsBy(int dx,
                        int dy);

private:
  QPointF lastBottomLeftPoint_;
  bool lastBottomLeftPointInitialized_;
};


// we want buttons that are horizontally as small as possible
class QPushButtonx
: public QPushButton
{
  Q_OBJECT

public:
  QPushButtonx(const QString& text,
               QWidget* = 0);
  virtual ~QPushButtonx(){}
};


// we want to have our own `stepBy' function for the zoom spin box
class QSpinBoxx
: public QSpinBox
{
  Q_OBJECT

public:
  void stepBy(int val);
  int valueFromText(const QString& text) const;
};


// end of custom_widgets.hpp
