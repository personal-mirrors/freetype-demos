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

  void applyToEngine(Engine* engine);
  void handleWheelResizeBySteps(int steps);
  void handleWheelResizeFromGrid(QWheelEvent* event);

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

  void createLayout();
  void createConnections();
  void setDefaults();
};


// end of fontsizeselector.hpp
