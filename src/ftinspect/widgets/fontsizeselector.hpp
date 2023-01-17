// fontsizeselector.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once

#include "customwidgets.hpp"

#include <vector>

#include <QBoxLayout>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QWheelEvent>
#include <QWidget>


class Engine;
class FontSizeSelector
: public QWidget
{
  Q_OBJECT

public:
  // For the continuous view mode, see `ZoomSpinBox`'s documentation.
  FontSizeSelector(QWidget* parent,
                   bool zoomNewLine,
                   bool continuousView);
  ~FontSizeSelector() override = default;

  enum Units : int
  {
    Units_px,
    Units_pt
  };

  //////// Getters
  double selectedSize();
  Units selectedUnit();
  double zoomFactor();

  //////// Setters
  void setSizePixel(int sizePixel);
  void setSizePoint(double sizePoint);
  void setZoomFactor(double zoomFactor);

  void reloadFromFont(Engine* engine);
  void applyToEngine(Engine* engine);
  void handleWheelResizeBySteps(int steps);
  void handleWheelZoomBySteps(int steps);
  void handleWheelResizeFromGrid(QWheelEvent* event);
  bool handleKeyEvent(QKeyEvent const* keyEvent);
  void installEventFilterForWidget(QWidget* widget);

protected:
  bool eventFilter(QObject* watched,
                   QEvent* event) override;

signals:
  void valueChanged();

private:
  QLabel* sizeLabel_;
  QLabel* dpiLabel_;
  QLabel* zoomLabel_;

  QDoubleSpinBox* sizeDoubleSpinBox_;
  QComboBox* unitsComboBox_;
  QSpinBox* dpiSpinBox_;
  ZoomSpinBox* zoomSpinBox_;

  // Sometimes we need to split 2 lines.
  QHBoxLayout* upLayout_;
  QHBoxLayout* downLayout_;
  QVBoxLayout* mainLayout_;

  bool continuousView_;
  double lastValue_;
  bool bitmapOnly_ = false;
  std::vector<int> fixedSizes_;

  void createLayout(bool zoomNewLine);
  void createConnections();
  void setDefaults(bool sizeOnly = false);

  void checkUnits();
  void checkFixedSizeAndEmit();
};


// end of fontsizeselector.hpp
