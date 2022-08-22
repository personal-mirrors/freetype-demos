// fontsizeselector.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QWidget>
#include <QBoxLayout>
#include <QWheelEvent>

class Engine;
class FontSizeSelector : public QWidget
{
  Q_OBJECT

public:
  FontSizeSelector(QWidget* parent);
  ~FontSizeSelector() override = default;

  enum Units : int
  {
    Units_px,
    Units_pt
  };

  double selectedSize();
  Units selectedUnit();
  void setSizePixel(int sizePixel);
  void setSizePoint(double sizePoint);

  void reloadFromFont(Engine* engine);
  void applyToEngine(Engine* engine);
  void handleWheelResizeBySteps(int steps);
  void handleWheelResizeFromGrid(QWheelEvent* event);
  bool handleKeyEvent(QKeyEvent const* keyEvent);
  void installEventFilterForWidget(QWidget* widget);

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

signals:
  void valueChanged();

private slots:
  void checkUnits();

private:
  QLabel* sizeLabel_;
  QLabel* dpiLabel_;

  QDoubleSpinBox* sizeDoubleSpinBox_;
  QComboBox* unitsComboBox_;
  QSpinBox* dpiSpinBox_;

  QHBoxLayout* layout_;

  double lastValue_;
  bool bitmapOnly_ = false;
  std::vector<int> fixedSizes_;

  void createLayout();
  void createConnections();
  void setDefaults(bool sizeOnly = false);

  void checkFixedSizeAndEmit();
};


// end of fontsizeselector.hpp
