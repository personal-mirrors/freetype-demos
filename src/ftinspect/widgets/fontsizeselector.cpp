// fontsizeselector.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "fontsizeselector.hpp"

#include "../engine/engine.hpp"

#include <algorithm>

FontSizeSelector::FontSizeSelector(QWidget* parent)
: QWidget(parent)
{
  createLayout();
  createConnections();
  setDefaults();
}


double
FontSizeSelector::selectedSize()
{
  return sizeDoubleSpinBox_->value();
}


FontSizeSelector::Units
FontSizeSelector::selectedUnit()
{
  return static_cast<Units>(unitsComboBox_->currentIndex());
}


void
FontSizeSelector::setSizePixel(int sizePixel)
{
  sizeDoubleSpinBox_->setValue(sizePixel);
  unitsComboBox_->setCurrentIndex(Units_px);
}


void
FontSizeSelector::setSizePoint(double sizePoint)
{
  sizeDoubleSpinBox_->setValue(sizePoint);
  unitsComboBox_->setCurrentIndex(Units_pt);
}


void
FontSizeSelector::reloadFromFont(Engine* engine)
{
  bitmapOnly_ = engine->currentFontBitmapOnly();
  fixedSizes_ = engine->currentFontFixedSizes();
  std::sort(fixedSizes_.begin(), fixedSizes_.end());
  if (fixedSizes_.empty())
    bitmapOnly_ = false; // Well this won't work...

  unitsComboBox_->setEnabled(!bitmapOnly_);

  {
    QSignalBlocker blocker(this);
    unitsComboBox_->setCurrentIndex(Units_px);
  }
  checkFixedSizeAndEmit();
}


void
FontSizeSelector::applyToEngine(Engine* engine)
{
  // Spinbox value cannot become negative
  engine->setDPI(dpiSpinBox_->value());

  if (unitsComboBox_->currentIndex() == Units_px)
    engine->setSizeByPixel(sizeDoubleSpinBox_->value());
  else
    engine->setSizeByPoint(sizeDoubleSpinBox_->value());
}


void
FontSizeSelector::handleWheelResizeBySteps(int steps)
{
  double sizeAfter = sizeDoubleSpinBox_->value()
                       + steps * sizeDoubleSpinBox_->singleStep();
  sizeAfter = std::max(sizeDoubleSpinBox_->minimum(),
                       std::min(sizeAfter, sizeDoubleSpinBox_->maximum()));
  sizeDoubleSpinBox_->setValue(sizeAfter);
}


void
FontSizeSelector::handleWheelResizeFromGrid(QWheelEvent* event)
{
  int numSteps = event->angleDelta().y() / 120;
  handleWheelResizeBySteps(numSteps);
}


bool
FontSizeSelector::handleKeyEvent(QKeyEvent const* keyEvent)
{
  if (!keyEvent)
    return false;
  auto modifiers = keyEvent->modifiers();
  auto key = keyEvent->key();
  if ((modifiers == Qt::ShiftModifier
       || modifiers == (Qt::ShiftModifier | Qt::KeypadModifier))
      && (key == Qt::Key_Plus 
          || key == Qt::Key_Minus
          || key == Qt::Key_Underscore
          || key == Qt::Key_Equal
          || key == Qt::Key_ParenRight))
  {
    if (key == Qt::Key_Plus || key == Qt::Key_Equal)
      handleWheelResizeBySteps(1);
    else if (key == Qt::Key_Minus
             || key == Qt::Key_Underscore)
      handleWheelResizeBySteps(-1);
    else if (key == Qt::Key_ParenRight)
      setDefaults(true);
    return true;
  }
  return false;
}


void
FontSizeSelector::installEventFilterForWidget(QWidget* widget)
{
  widget->installEventFilter(this);
}


bool
FontSizeSelector::eventFilter(QObject* watched,
                              QEvent* event)
{
  if (event->type() == QEvent::KeyPress)
  {
    auto keyEvent = dynamic_cast<QKeyEvent*>(event);
    if (handleKeyEvent(keyEvent))
      return true;
  }
  return QWidget::eventFilter(watched, event);
}


void
FontSizeSelector::checkUnits()
{
  int index = unitsComboBox_->currentIndex();

  if (index == Units_px)
  {
    dpiLabel_->setEnabled(false);
    dpiSpinBox_->setEnabled(false);
    sizeDoubleSpinBox_->setSingleStep(1);

    QSignalBlocker blocker(sizeDoubleSpinBox_);
    sizeDoubleSpinBox_->setValue(qRound(sizeDoubleSpinBox_->value()));
  }
  else
  {
    dpiLabel_->setEnabled(true);
    dpiSpinBox_->setEnabled(true);
    sizeDoubleSpinBox_->setSingleStep(0.5);
  }

  emit valueChanged();
}


void
FontSizeSelector::createLayout()
{
  sizeLabel_ = new QLabel(tr("Size "), this);
  sizeLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  sizeDoubleSpinBox_ = new QDoubleSpinBox;
  sizeDoubleSpinBox_->setAlignment(Qt::AlignRight);
  sizeDoubleSpinBox_->setDecimals(1);
  sizeDoubleSpinBox_->setRange(1, 500);
  sizeLabel_->setBuddy(sizeDoubleSpinBox_);

  unitsComboBox_ = new QComboBox(this);
  unitsComboBox_->insertItem(Units_px, "px");
  unitsComboBox_->insertItem(Units_pt, "pt");

  dpiLabel_ = new QLabel(tr("DPI "), this);
  dpiLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  dpiSpinBox_ = new QSpinBox(this);
  dpiSpinBox_->setAlignment(Qt::AlignRight);
  dpiSpinBox_->setRange(10, 600);
  dpiLabel_->setBuddy(dpiSpinBox_);

  layout_ = new QHBoxLayout;

  layout_->addStretch(1);
  layout_->addWidget(sizeLabel_);
  layout_->addWidget(sizeDoubleSpinBox_);
  layout_->addWidget(unitsComboBox_);
  layout_->addStretch(1);
  layout_->addWidget(dpiLabel_);
  layout_->addWidget(dpiSpinBox_);
  layout_->addStretch(1);

  setLayout(layout_);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}


void
FontSizeSelector::createConnections()
{
  connect(sizeDoubleSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &FontSizeSelector::checkFixedSizeAndEmit);
  connect(unitsComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &FontSizeSelector::checkUnits);
  connect(dpiSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged),
          this, &FontSizeSelector::checkFixedSizeAndEmit);
}


void
FontSizeSelector::setDefaults(bool sizeOnly)
{
  lastValue_ = 20;
  sizeDoubleSpinBox_->setValue(lastValue_);
  if (sizeOnly)
    return;
  dpiSpinBox_->setValue(96);
  checkUnits();
}


void
FontSizeSelector::checkFixedSizeAndEmit()
{
  if (bitmapOnly_ && !fixedSizes_.empty())
  {
    auto newValue = sizeDoubleSpinBox_->value();
    auto intNewValue = static_cast<int>(newValue);
    if (newValue != static_cast<double>(intNewValue))
    {
      sizeDoubleSpinBox_->setValue(intNewValue);
      return; // Don't emit.
    }

    if (!std::binary_search(fixedSizes_.begin(), fixedSizes_.end(), newValue))
    {
      // Value not available, find next value.
      if (intNewValue > lastValue_)
      {
        // find next larger value...
        auto it = std::upper_bound(fixedSizes_.begin(), fixedSizes_.end(),
                                   lastValue_);
        if (it == fixedSizes_.end())
          sizeDoubleSpinBox_->setValue(lastValue_);
        else
          sizeDoubleSpinBox_->setValue(*it);
      }
      else
      {
        // find next smaller value...
        auto it = std::lower_bound(fixedSizes_.begin(), fixedSizes_.end(),
                                   lastValue_);

        // there's no element >= lastValue => all elements < last value
        if (it == fixedSizes_.begin())
          sizeDoubleSpinBox_->setValue(fixedSizes_.front());
        else
          sizeDoubleSpinBox_->setValue(*(it - 1));
      }
      return;
    }
  }

  lastValue_ = sizeDoubleSpinBox_->value();
  emit valueChanged();
}


// end of fontsizeselector.cpp
