// glyphindexselector.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "glyphindexselector.hpp"
#include "../uihelper.hpp"

#include <climits>


GlyphIndexSelector::GlyphIndexSelector(QWidget* parent)
: QWidget(parent)
{
  numberRenderer_ = &GlyphIndexSelector::renderNumberDefault;

  createLayout();
  createConnections();
  showingCount_ = 0;
}


void
GlyphIndexSelector::setMinMax(int min,
                              int max)
{
  // Don't emit events during setting.
  auto eventState = blockSignals(true);
  indexSpinBox_->setMinimum(min);
  indexSpinBox_->setMaximum(qBound(0, max, INT_MAX));
  indexSpinBox_->setValue(qBound(indexSpinBox_->minimum(),
                                 indexSpinBox_->value(),
                                 indexSpinBox_->maximum()));
  blockSignals(eventState);

  updateLabel();
}


void
GlyphIndexSelector::setShowingCount(int showingCount)
{
  showingCount_ = showingCount;
  updateLabel();
}


void
GlyphIndexSelector::setSingleMode(bool singleMode)
{
  singleMode_ = singleMode;
  updateLabel();
}


void
GlyphIndexSelector::setCurrentIndex(int index,
                                    bool forceUpdate)
{
  // To avoid unnecessary updates: if force-update is enabled,
  // `setValue` shouldn't trigger the update signal from `this`.
  // However, we still need `updateLabel`, so block `this` only.
  auto state = blockSignals(forceUpdate);
  indexSpinBox_->setValue(index);
  blockSignals(state);

  if (forceUpdate)
    emit currentIndexChanged(indexSpinBox_->value());
}


int
GlyphIndexSelector::currentIndex()
{
  return indexSpinBox_->value();
}


void
GlyphIndexSelector::setNumberRenderer(std::function<QString(int)> renderer)
{
  numberRenderer_ = std::move(renderer);
}


void
GlyphIndexSelector::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  auto minimumWidth = minimumSizeHint().width();
  if (toEndButton_->isVisible())
  {
    if (width() < minimumWidth)
      navigationWidget_->setVisible(false);
  }
  else if (navigationWidget_->minimumSizeHint().width() + minimumWidth
           <= width())
    navigationWidget_->setVisible(true);
}


void
GlyphIndexSelector::adjustIndex(int delta)
{
  {
    QSignalBlocker blocker(this);
    indexSpinBox_->setValue(qBound(indexSpinBox_->minimum(),
                                   indexSpinBox_->value() + delta,
                                   indexSpinBox_->maximum()));
  }
  emitValueChanged();
}


void
GlyphIndexSelector::emitValueChanged()
{
  emit currentIndexChanged(indexSpinBox_->value());
  updateLabel();
}


void
GlyphIndexSelector::updateLabel()
{
  if (singleMode_)
    indexLabel_->setText(QString("%1\nLimit: %2")
                           .arg(numberRenderer_(indexSpinBox_->value()))
                           .arg(numberRenderer_(indexSpinBox_->maximum())));
  else
    indexLabel_->setText(
      QString("%1~%2\nLimit: %4")
        .arg(numberRenderer_(indexSpinBox_->value()))
        .arg(numberRenderer_(
          qBound(indexSpinBox_->value(),
                 indexSpinBox_->value() + showingCount_ - 1, INT_MAX)))
        .arg(numberRenderer_(indexSpinBox_->maximum())));
}


void
GlyphIndexSelector::createLayout()
{
  navigationWidget_ = new QWidget(this);
  toStartButton_ = new QPushButton("|<", this);
  toM1000Button_ = new QPushButton("-1000", this);
  toM100Button_ = new QPushButton("-100", this);
  toM10Button_ = new QPushButton("-10", this);
  toM1Button_ = new QPushButton("-1", this);
  toP1Button_ = new QPushButton("+1", this);
  toP10Button_ = new QPushButton("+10", this);
  toP100Button_ = new QPushButton("+100", this);
  toP1000Button_ = new QPushButton("+1000", this);
  toEndButton_ = new QPushButton(">|", this);

  indexSpinBox_ = new QSpinBox(this);
  indexSpinBox_->setCorrectionMode(QAbstractSpinBox::CorrectToNearestValue);
  indexSpinBox_->setButtonSymbols(QAbstractSpinBox::NoButtons);
  indexSpinBox_->setRange(0, 0);
  indexSpinBox_->setFixedWidth(80);
  indexSpinBox_->setWrapping(false);
  indexSpinBox_->setKeyboardTracking(false);

  indexLabel_ = new QLabel("0\nLimit: 0");
  indexLabel_->setMinimumWidth(200);

  setButtonNarrowest(toStartButton_);
  setButtonNarrowest(toM1000Button_);
  setButtonNarrowest(toM100Button_);
  setButtonNarrowest(toM10Button_);
  setButtonNarrowest(toM1Button_);
  setButtonNarrowest(toP1Button_);
  setButtonNarrowest(toP10Button_);
  setButtonNarrowest(toP100Button_);
  setButtonNarrowest(toP1000Button_);
  setButtonNarrowest(toEndButton_);

  // Toltips
  indexSpinBox_->setToolTip("Current glyph index.");
  indexLabel_->setToolTip("Current glyph index/range and the max index.");

  // Layouting
  navigationLayout_ = new QHBoxLayout;
  navigationLayout_->setSpacing(0);
  navigationLayout_->addWidget(toStartButton_);
  navigationLayout_->addWidget(toM1000Button_);
  navigationLayout_->addWidget(toM100Button_);
  navigationLayout_->addWidget(toM10Button_);
  navigationLayout_->addWidget(toM1Button_);
  navigationLayout_->addWidget(toP1Button_);
  navigationLayout_->addWidget(toP10Button_);
  navigationLayout_->addWidget(toP100Button_);
  navigationLayout_->addWidget(toP1000Button_);
  navigationLayout_->addWidget(toEndButton_);
  navigationWidget_->setLayout(navigationLayout_);

  layout_ = new QHBoxLayout;
  layout_->setSpacing(0);
  layout_->addStretch(3);
  layout_->addWidget(navigationWidget_);
  layout_->addStretch(1);
  layout_->addWidget(indexSpinBox_);
  layout_->addStretch(1);
  layout_->addWidget(indexLabel_);
  layout_->addStretch(3);

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  setLayout(layout_);
}


void
GlyphIndexSelector::createConnections()
{
  connect(indexSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged),
          this, &GlyphIndexSelector::emitValueChanged);

  glyphNavigationMapper_ = new QSignalMapper(this);
  connect(glyphNavigationMapper_, &QSignalMapper::mappedInt,
          this, &GlyphIndexSelector::adjustIndex);

  connect(toStartButton_, &QPushButton::clicked,
          glyphNavigationMapper_, QOverload<>::of(&QSignalMapper::map));
  connect(toM1000Button_, &QPushButton::clicked,
          glyphNavigationMapper_, QOverload<>::of(&QSignalMapper::map));
  connect(toM100Button_, &QPushButton::clicked,
          glyphNavigationMapper_, QOverload<>::of(&QSignalMapper::map));
  connect(toM10Button_, &QPushButton::clicked,
          glyphNavigationMapper_, QOverload<>::of(&QSignalMapper::map));
  connect(toM1Button_, &QPushButton::clicked,
          glyphNavigationMapper_, QOverload<>::of(&QSignalMapper::map));
  connect(toP1Button_, &QPushButton::clicked,
          glyphNavigationMapper_, QOverload<>::of(&QSignalMapper::map));
  connect(toP10Button_, &QPushButton::clicked,
          glyphNavigationMapper_, QOverload<>::of(&QSignalMapper::map));
  connect(toP100Button_, &QPushButton::clicked,
          glyphNavigationMapper_, QOverload<>::of(&QSignalMapper::map));
  connect(toP1000Button_, &QPushButton::clicked,
          glyphNavigationMapper_, QOverload<>::of(&QSignalMapper::map));
  connect(toEndButton_, &QPushButton::clicked,
          glyphNavigationMapper_, QOverload<>::of(&QSignalMapper::map));

  glyphNavigationMapper_->setMapping(toStartButton_, -0x10000);
  glyphNavigationMapper_->setMapping(toM1000Button_, -1000);
  glyphNavigationMapper_->setMapping(toM100Button_, -100);
  glyphNavigationMapper_->setMapping(toM10Button_, -10);
  glyphNavigationMapper_->setMapping(toM1Button_, -1);
  glyphNavigationMapper_->setMapping(toP1Button_, 1);
  glyphNavigationMapper_->setMapping(toP10Button_, 10);
  glyphNavigationMapper_->setMapping(toP100Button_, 100);
  glyphNavigationMapper_->setMapping(toP1000Button_, 1000);
  glyphNavigationMapper_->setMapping(toEndButton_, 0x10000);
}


QString
GlyphIndexSelector::renderNumberDefault(int i)
{
  return QString::number(i);
}


// end of glyphindexselector.cpp
