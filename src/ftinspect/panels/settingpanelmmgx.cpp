// settingpanelmmgx.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "settingpanelmmgx.hpp"

#include <QScrollBar>

#include "../engine/engine.hpp"
#include "../uihelper.hpp"


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

  // This won't trigger unexpected updating since signals are blocked.
  for (size_t i = 0; i < minSize; ++i)
    itemWidgets_[i]->updateInfo(currentAxes_[i]);

  if (newSize < oldSize)
  {
    for (size_t i = newSize; i < oldSize; ++i)
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
              [this, i] { itemChanged(i); });
    }
  }
  checkHidden();
  retrieveValues();
  applySettings();
}


void
SettingPanelMMGX::applySettings()
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
  groupingCheckBox_ = new QCheckBox(tr("Grouping"), this);
  resetDefaultButton_ = new QPushButton(tr("Reset Default"), this);
  itemsListWidget_ = new QWidget(this);
  scrollArea_ = new UnboundScrollArea(this);

  scrollArea_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
  scrollArea_->setWidget(itemsListWidget_);
  scrollArea_->setWidgetResizable(true);
  itemsListWidget_->setAutoFillBackground(false);

  mainLayout_ = new QVBoxLayout;
  listLayout_ = new QVBoxLayout;
  listWrapperLayout_ = new QVBoxLayout;

  listLayout_->setSpacing(0);
  listLayout_->setContentsMargins(0, 0, 0, 0);
  itemsListWidget_->setContentsMargins(0, 0, 0, 0);

  itemsListWidget_->setLayout(listWrapperLayout_);

  listWrapperLayout_->addLayout(listLayout_);
  listWrapperLayout_->addStretch(1);

  mainLayout_->addWidget(showHiddenCheckBox_);
  mainLayout_->addWidget(groupingCheckBox_);
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
  connect(groupingCheckBox_, &QCheckBox::clicked,
          this, &SettingPanelMMGX::checkGrouping);
}


void
SettingPanelMMGX::retrieveValues()
{
  currentValues_.resize(currentAxes_.size());
  for (unsigned i = 0; i < currentAxes_.size(); ++i)
    currentValues_[i] = itemWidgets_[i]->value();
}


void
SettingPanelMMGX::itemChanged(size_t index)
{
  if (groupingCheckBox_->isChecked()
      && index < currentAxes_.size()
      && index < itemWidgets_.size())
  {
    auto tag = currentAxes_[index].tag;
    auto value = itemWidgets_[index]->value();
    for (size_t i = 0; i < itemWidgets_.size(); ++i)
      if (i != index && currentAxes_[i].tag == tag)
        itemWidgets_[i]->setValue(value);
  }

  retrieveValues();
  emit mmgxCoordsChanged();
}


void
SettingPanelMMGX::resetDefaultClicked()
{
  for (auto w : itemWidgets_)
    w->resetDefault();

  retrieveValues();
  emit mmgxCoordsChanged();
}


void
SettingPanelMMGX::checkGrouping()
{
  if (!groupingCheckBox_->isChecked())
    return;
  auto maxIndex = std::max(itemWidgets_.size(), currentAxes_.size());
  for (size_t i = maxIndex - 1; ; --i)
  {
    if (!currentAxes_[i].hidden)
    {
      auto tag = currentAxes_[i].tag;
      auto value = itemWidgets_[i]->value();
      for (size_t j = 0; j < maxIndex; ++j)
        if (j != i && currentAxes_[j].tag == tag)
          itemWidgets_[j]->setValue(value);
    }

    if (i == 0)
      break;
  }

  retrieveValues();
  emit mmgxCoordsChanged();
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
  valueValidator_->setRange(info.minimum, info.maximum, 10);

  if (info.hidden)
    nameLabel_->setText("<i>" + info.name + "</i>");
  else
    nameLabel_->setText(info.name);

  // To keep things simple, we use 1/1024 of the span between the minimum
  // and the maximum as one step on the slider.
  slider_->setMinimum(0);
  slider_->setTickInterval(1);
  slider_->setMaximum(1024);

  resetDefault();
}


void
MMGXSettingItem::setValue(FT_Fixed value)
{
  actualValue_ = value;
  updateSlider();
  updateLineEdit();
}


void
MMGXSettingItem::resetDefault()
{
  setValue(static_cast<FT_Fixed>(axisInfo_.def * 65536.0));
}


void
MMGXSettingItem::createLayout()
{
  nameLabel_ = new QLabel(this);

  // 1/1024 = 0.0009765625
  valueValidator_ = new QDoubleValidator(0, 0, 10, this);
  valueValidator_->setNotation(QDoubleValidator::StandardNotation);

  valueLineEdit_ = new QLineEdit("0", this);
  valueLineEdit_->setValidator(valueValidator_);

  slider_ = new QSlider(this);
  slider_->setOrientation(Qt::Horizontal);

  resetDefaultButton_ = new QPushButton(this);
  resetDefaultButton_->setText(tr("Def"));
  setButtonNarrowest(resetDefaultButton_);

  nameLabel_->setToolTip(tr("Axis name or tag"));
  slider_->setToolTip(tr(
    "Axis value (max precision: 1/1024 of the value range)"));
  valueLineEdit_->setToolTip(tr("Axis value"));
  resetDefaultButton_->setToolTip(tr("Reset axis to default"));

  mainLayout_ = new QGridLayout();

  mainLayout_->addWidget(nameLabel_, 0, 0, 1, 2);
  mainLayout_->addWidget(slider_, 1, 0, 1, 2);
  mainLayout_->addWidget(valueLineEdit_, 2, 0, 1, 1, Qt::AlignVCenter);
  mainLayout_->addWidget(resetDefaultButton_, 2, 1, 1, 1, Qt::AlignVCenter);

  mainLayout_->setSpacing(4);
  mainLayout_->setContentsMargins(4, 4, 4, 4);
  setContentsMargins(0, 0, 0, 0);
  setLayout(mainLayout_);
  setFrameShape(StyledPanel);
}


void
MMGXSettingItem::createConnections()
{
  connect(slider_, &QSlider::valueChanged,
          this, &MMGXSettingItem::sliderValueChanged);
  connect(valueLineEdit_, &QLineEdit::editingFinished,
          this, &MMGXSettingItem::lineEditChanged);
  connect(resetDefaultButton_, &QToolButton::clicked,
          this, &MMGXSettingItem::resetDefaultSingle);
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

  updateLineEdit();
  emit valueChanged();
}


void
MMGXSettingItem::lineEditChanged()
{
  bool succ = false;
  auto newValue = valueLineEdit_->text().toDouble(&succ);
  if (!succ || newValue > axisInfo_.maximum || newValue < axisInfo_.minimum)
  {
    updateLineEdit();
    return;
  }

  actualValue_ = static_cast<FT_Fixed>(newValue / 65536.0);

  updateSlider();
  emit valueChanged();
}


void
MMGXSettingItem::updateLineEdit()
{
  QSignalBlocker blocker(valueLineEdit_);
  valueLineEdit_->setText(QString::number(actualValue_ / 65536.0));
}


void
MMGXSettingItem::updateSlider()
{
  QSignalBlocker blocker(slider_);
  slider_->setValue(
    static_cast<int>((actualValue_ / 65536.0 - axisInfo_.minimum)
                     / (axisInfo_.maximum - axisInfo_.minimum)
                     * 1024));
}


void
MMGXSettingItem::resetDefaultSingle()
{
  resetDefault();
  emit valueChanged();
}


// end of settingpanelmmgx.cpp
