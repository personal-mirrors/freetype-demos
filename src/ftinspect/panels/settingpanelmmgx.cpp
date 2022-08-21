// settingpanelmmgx.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "settingpanelmmgx.hpp"

#include <QScrollBar>

#include "../engine/engine.hpp"

SettingPanelMMGX::SettingPanelMMGX(QWidget* parent,
                                   Engine* engine)
: QWidget(parent),
  engine_(engine)
{
  createLayout();
  createConnections();
}


void
SettingPanelMMGX::reloadFont()
{
  setEnabled(engine_->currentFontMMGXState() != MMGXState::NoMMGX);
  if (currentAxes_ == engine_->currentFontMMGXAxes())
    return;

  currentAxes_ = engine_->currentFontMMGXAxes();

  auto newSize = currentAxes_.size();
  auto oldSize = itemWidgets_.size();
  auto minSize = std::min(newSize, oldSize);

  // This won't trigger unexpected updating since signals are blocked
  for (size_t i = 0; i < minSize; ++i)
    itemWidgets_[i]->updateInfo(currentAxes_[i]);

  if (newSize < oldSize)
  {
    for (size_t i = oldSize; i < newSize; ++i)
    {
      auto w = itemWidgets_[i];
      disconnect(w);
      listLayout_->removeWidget(w);
      delete w;
    }
    itemWidgets_.resize(newSize);
  }
  else if (newSize > oldSize)
  {
    itemWidgets_.resize(newSize);
    for (size_t i = oldSize; i < newSize; ++i)
    {
      auto w = new MMGXSettingItem(this);
      itemWidgets_[i] = w;
      w->updateInfo(currentAxes_[i]);
      listLayout_->addWidget(w);
      connect(w, &MMGXSettingItem::valueChanged,
              this, &SettingPanelMMGX::itemChanged);
    }
  }
  checkHidden();
}


void
SettingPanelMMGX::syncSettings()
{
  engine_->reloadFont();
  engine_->applyMMGXDesignCoords(currentValues_.data(),
                                 currentValues_.size());
}


void
SettingPanelMMGX::checkHidden()
{
  if (itemWidgets_.size() < currentAxes_.size())
    return; // This should never happen!
  for (int i = 0; static_cast<size_t>(i) < currentAxes_.size(); ++i)
    itemWidgets_[i]->setVisible(showHiddenCheckBox_->isChecked()
                                || !currentAxes_[i].hidden);
}


void
SettingPanelMMGX::createLayout()
{
  showHiddenCheckBox_ = new QCheckBox(tr("Show Hidden"), this);
  resetDefaultButton_ = new QPushButton(tr("Reset Default"), this);
  itemsListWidget_ = new QWidget(this);
  scrollArea_ = new UnboundScrollArea(this);

  scrollArea_->setWidget(itemsListWidget_);
  scrollArea_->setWidgetResizable(true);
  scrollArea_->setStyleSheet("QScrollArea {background-color:transparent;}");
  itemsListWidget_->setStyleSheet("background-color:transparent;");

  mainLayout_ = new QVBoxLayout;
  listLayout_ = new QVBoxLayout;
  listWrapperLayout_ = new QVBoxLayout;

  listLayout_->setContentsMargins(0, 0, 0, 0);
  itemsListWidget_->setContentsMargins(0, 0, 0, 0);

  itemsListWidget_->setLayout(listWrapperLayout_);

  listWrapperLayout_->addLayout(listLayout_);
  listWrapperLayout_->addStretch(1);

  mainLayout_->addWidget(showHiddenCheckBox_);
  mainLayout_->addWidget(resetDefaultButton_);
  mainLayout_->addWidget(scrollArea_, 1);

  setLayout(mainLayout_);
}


void
SettingPanelMMGX::createConnections()
{
  connect(showHiddenCheckBox_, &QCheckBox::clicked,
          this, &SettingPanelMMGX::checkHidden);
  connect(resetDefaultButton_, &QCheckBox::clicked,
          this, &SettingPanelMMGX::resetDefaultClicked);
}


void
SettingPanelMMGX::itemChanged()
{
  currentValues_.resize(currentAxes_.size());
  for (unsigned i = 0; i < currentAxes_.size(); ++i)
    currentValues_[i] = itemWidgets_[i]->value();

  emit mmgxCoordsChanged();
}


void
SettingPanelMMGX::resetDefaultClicked()
{
  for (auto w : itemWidgets_)
    w->resetDefault();
  itemChanged();
}


MMGXSettingItem::MMGXSettingItem(QWidget* parent)
: QFrame(parent)
{
  createLayout();
  createConnections();
}


void
MMGXSettingItem::updateInfo(MMGXAxisInfo& info)
{
  axisInfo_ = info;
  if (info.hidden)
    nameLabel_->setText("<i>" + info.name + "</i>");
  else
    nameLabel_->setText(info.name);

  // To keep things simple, we will use 1/1024 of the span between the min and
  // max as one step on the slider.

  slider_->setMinimum(0);
  slider_->setTickInterval(1);
  slider_->setMaximum(1024);

  resetDefault();
}


void
MMGXSettingItem::resetDefault()
{
  QSignalBlocker blocker(this);
  slider_->setValue(static_cast<int>((axisInfo_.def - axisInfo_.minimum)
                                     / (axisInfo_.maximum - axisInfo_.minimum)
                                     * 1024));
}


void
MMGXSettingItem::createLayout()
{
  nameLabel_ = new QLabel(this);
  valueLabel_ = new QLabel(this);
  slider_ = new QSlider(this);
  slider_->setOrientation(Qt::Horizontal);

  mainLayout_ = new QGridLayout();

  mainLayout_->addWidget(nameLabel_, 0, 0);
  mainLayout_->addWidget(valueLabel_, 0, 1, 1, 1, Qt::AlignRight);
  mainLayout_->addWidget(slider_, 1, 0, 1, 2);

  mainLayout_->setContentsMargins(4, 4, 4, 4);
  setContentsMargins(4, 4, 4, 4);
  setLayout(mainLayout_);
  setFrameShape(StyledPanel);
}


void
MMGXSettingItem::createConnections()
{
  connect(slider_, &QSlider::valueChanged,
          this, &MMGXSettingItem::sliderValueChanged);
}


void
MMGXSettingItem::sliderValueChanged()
{
  auto value = slider_->value() / 1024.0
               * (axisInfo_.maximum - axisInfo_.minimum)
               + axisInfo_.minimum;
  actualValue_ = static_cast<FT_Fixed>(value * 65536.0);

  if (axisInfo_.isMM)
    actualValue_ = FT_RoundFix(actualValue_);
  else
  {
    double x = actualValue_ / 65536.0 * 100.0;
    x += x < 0.0 ? -0.5 : 0.5;
    x = static_cast<int>(x);
    x = x / 100.0 * 65536.0;
    x += x < 0.0 ? -0.5 : 0.5;

    actualValue_ = static_cast<FT_Fixed>(x);
  }

  valueLabel_->setText(QString::number(actualValue_ / 65536.0));

  emit valueChanged();
}


// end of settingpanelmmgx.cpp
