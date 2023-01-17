// fontsizeselector.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "fontsizeselector.hpp"
#include "../engine/engine.hpp"

#include <algorithm>


FontSizeSelector::FontSizeSelector(QWidget* parent,
                                   bool zoomNewLine,
                                   bool continuousView)
: QWidget(parent),
  continuousView_(continuousView)
{
  createLayout(zoomNewLine);
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


double
FontSizeSelector::zoomFactor()
{
  if (continuousView_)
    return zoomSpinBox_->value();
  return static_cast<int>(zoomSpinBox_->value());
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
FontSizeSelector::setZoomFactor(double zoomFactor)
{
  if (continuousView_)
    zoomSpinBox_->setValue(zoomFactor);
  zoomSpinBox_->setValue(static_cast<int>(zoomFactor));
}


void
FontSizeSelector::reloadFromFont(Engine* engine)
{
  engine->reloadFont();
  bitmapOnly_ = engine->currentFontBitmapOnly();
  fixedSizes_ = engine->currentFontFixedSizes();
  std::sort(fixedSizes_.begin(), fixedSizes_.end());
  if (fixedSizes_.empty())
    bitmapOnly_ = false; // Well this won't work...

  unitsComboBox_->setEnabled(!bitmapOnly_);
  sizeDoubleSpinBox_->setKeyboardTracking(!bitmapOnly_);

  if (bitmapOnly_)
  {
    QSignalBlocker blocker(this);
    unitsComboBox_->setCurrentIndex(Units_px);
  }
  checkFixedSizeAndEmit();
}


void
FontSizeSelector::applyToEngine(Engine* engine)
{
  // Spinbox value cannot become negative.
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
FontSizeSelector::handleWheelZoomBySteps(int steps)
{
  double zoomAfter = zoomSpinBox_->value()
                     + steps * zoomSpinBox_->singleStep();
  zoomAfter = std::max(zoomSpinBox_->minimum(),
                       std::min(zoomAfter, zoomSpinBox_->maximum()));
  zoomSpinBox_->setValue(zoomAfter);
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
FontSizeSelector::createLayout(bool zoomNewLine)
{
  sizeLabel_ = new QLabel(tr("Size "), this);
  sizeLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  sizeDoubleSpinBox_ = new QDoubleSpinBox(this);
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

  zoomLabel_ = new QLabel(tr("Zoom Factor "), this);
  zoomLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  zoomSpinBox_ = new ZoomSpinBox(this, continuousView_);
  zoomSpinBox_->setAlignment(Qt::AlignRight);
  zoomLabel_->setBuddy(zoomSpinBox_);

  // Tooltips
  sizeDoubleSpinBox_->setToolTip(tr(
    "Size value (will be limited to available sizes if\n"
    "the current font is not scalable)."));
  unitsComboBox_->setToolTip(tr(
    "Unit for the size value (force to pixel if\n"
    "the current font is not scalable)."));
  dpiSpinBox_->setToolTip(tr(
    "DPI for the point size value (only valid when the unit is point)."));
  zoomSpinBox_->setToolTip(tr("Adjust zoom."));

  // Layouting
  mainLayout_ = new QVBoxLayout;
  upLayout_ = new QHBoxLayout;
  upLayout_->addStretch(1);
  upLayout_->addWidget(sizeLabel_);
  upLayout_->addWidget(sizeDoubleSpinBox_);
  upLayout_->addWidget(unitsComboBox_);
  upLayout_->addStretch(1);
  upLayout_->addWidget(dpiLabel_);
  upLayout_->addWidget(dpiSpinBox_);
  upLayout_->addStretch(1);
  if (!zoomNewLine)
  {
    upLayout_->addWidget(zoomLabel_);
    upLayout_->addWidget(zoomSpinBox_);
    upLayout_->addStretch(1);
    mainLayout_->addLayout(upLayout_);
  }
  else
  {
    downLayout_ = new QHBoxLayout;
    downLayout_->addWidget(zoomLabel_);
    downLayout_->addWidget(zoomSpinBox_, 1);
    mainLayout_->addLayout(upLayout_);
    mainLayout_->addLayout(downLayout_);
  }

  setLayout(mainLayout_);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}


void
FontSizeSelector::createConnections()
{
  connect(sizeDoubleSpinBox_,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this,
          &FontSizeSelector::checkFixedSizeAndEmit);
  connect(unitsComboBox_,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this,
          &FontSizeSelector::checkUnits);
  connect(dpiSpinBox_,
          QOverload<int>::of(&QSpinBox::valueChanged),
          this,
          &FontSizeSelector::checkFixedSizeAndEmit);
  connect(zoomSpinBox_,
          QOverload<double>::of(&ZoomSpinBox::valueChanged),
          this,
          &FontSizeSelector::valueChanged);
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
        // Find next larger value...
        auto it = std::upper_bound(fixedSizes_.begin(), fixedSizes_.end(),
                                   lastValue_);
        if (it == fixedSizes_.end())
          sizeDoubleSpinBox_->setValue(lastValue_);
        else
          sizeDoubleSpinBox_->setValue(*it);
      }
      else
      {
        // Find next smaller value...
        auto it = std::lower_bound(fixedSizes_.begin(), fixedSizes_.end(),
                                   lastValue_);

        // There's no element >= lastValue => all elements < last value.
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
