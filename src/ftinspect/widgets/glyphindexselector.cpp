// glyphindexselector.cpp

// Copyright (C) 2022 Charlie Jiang.

#include "glyphindexselector.hpp"

#include "../uihelper.hpp"

GlyphIndexSelector::GlyphIndexSelector(QWidget* parent)
: QWidget(parent)
{
  createLayout();
  createConnections();
}


void
GlyphIndexSelector::setMin(int min)
{
  indexSpinBox_->setMinimum(min);
  indexSpinBox_->setValue(qBound(indexSpinBox_->minimum(),
                                 indexSpinBox_->value(),
                                 indexSpinBox_->maximum()));
  // spinBoxChanged will be automatically called
}


void
GlyphIndexSelector::setMax(int max)
{
  indexSpinBox_->setMaximum(max);
  indexSpinBox_->setValue(qBound(indexSpinBox_->minimum(),
                                 indexSpinBox_->value(),
                                 indexSpinBox_->maximum()));
  // spinBoxChanged will be automatically called
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
GlyphIndexSelector::setCurrentIndex(int index, bool forceUpdate)
{
  indexSpinBox_->setValue(index);
  updateLabel();
  if (forceUpdate)
    emit currentIndexChanged(indexSpinBox_->value());
}


int
GlyphIndexSelector::getCurrentIndex()
{
  return indexSpinBox_->value();
}


void
GlyphIndexSelector::adjustIndex(int delta)
{
  indexSpinBox_->setValue(qBound(indexSpinBox_->minimum(),
                                 indexSpinBox_->value() + delta,
                                 indexSpinBox_->maximum()));
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
                             .arg(indexSpinBox_->value())
                             .arg(indexSpinBox_->maximum()));
  else
    indexLabel_->setText(QString("%1~%2\nCount: %3\nLimit: %4")
                             .arg(indexSpinBox_->value())
                             .arg(indexSpinBox_->value() + showingCount_ - 1)
                             .arg(showingCount_, indexSpinBox_->maximum()));
}


void
GlyphIndexSelector::createLayout()
{
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
  indexSpinBox_->setWrapping(true);

  indexLabel_ = new QLabel("0\nCount: 0\nLimit: 0");

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

  navigationLayout_ = new QHBoxLayout;
  navigationLayout_->setSpacing(0);
  navigationLayout_->addStretch(3);
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
  navigationLayout_->addStretch(1);
  navigationLayout_->addWidget(indexSpinBox_);
  navigationLayout_->addStretch(1);
  navigationLayout_->addWidget(indexLabel_);
  navigationLayout_->addStretch(3);

  setLayout(navigationLayout_);
}

void
GlyphIndexSelector::createConnections()
{
  connect(indexSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), 
          this, &GlyphIndexSelector::emitValueChanged);

  glyphNavigationMapper_ = new QSignalMapper(this);
  connect(glyphNavigationMapper_, SIGNAL(mapped(int)), SLOT(adjustIndex(int)));

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


// end of glyphindexselector.cpp
