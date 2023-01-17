// customwidgets.hpp

// Copyright (C) 2016-2023 by
// Werner Lemberg.

#pragma once

#include <QComboBox>
#include <QGraphicsView>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QString>


// We need to define a series of custom Qt widgets to satisfy our needs.
// Basically, those custom widgets are derived classes from Qt-provided
// components, with minor changes.  Because all those derived classes are
// pretty tiny and not core logic, they are organized into one single
// hpp/cpp file pair.

// We want to anchor the view at the bottom left corner while the windows
// gets resized.
class QGraphicsViewx
: public QGraphicsView
{
  Q_OBJECT

public:
  QGraphicsViewx(QWidget* parent);

signals:
  void shiftWheelEvent(QWheelEvent* event);
  void ctrlWheelEvent(QWheelEvent* event);

protected:
  void wheelEvent(QWheelEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void scrollContentsBy(int dx,
                        int dy) override;

private:
  QPointF lastBottomLeftPoint_;
  bool lastBottomLeftPointInitialized_;
};


// We want to have our own `stepBy` function for the zoom spin box.
class ZoomSpinBox
: public QDoubleSpinBox
{
  Q_OBJECT

public:
  // The ContinuousView mode for `ZoomSpinBox` will change the range to
  // 0.25~50, and the single step to 0.25.
  ZoomSpinBox(QWidget* parent,
              bool continuousView);
  void stepBy(int val) override;
  double valueFromText(const QString& text) const override;

private:
  bool continuousView_;
};


// https://bugreports.qt.io/browse/QTBUG-10459
// https://phabricator.kde.org/D14692
class UnboundScrollArea
: public QScrollArea
{
  Q_OBJECT

public:
  UnboundScrollArea(QWidget* parent);
  QSize sizeHint() const override;
};


// end of customwidgets.hpp
