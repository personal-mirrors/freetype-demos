// settingpanelmmgx.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include "../engine/mmgx.hpp"
#include "../widgets/customwidgets.hpp"

#include <QWidget>
#include <QGridLayout>
#include <QBoxLayout>
#include <QCheckBox>
#include <QScrollArea>
#include <QSlider>
#include <QFrame>
#include <QLabel>

#include <freetype/fttypes.h>

class Engine;

class MMGXSettingItem;
class SettingPanelMMGX
: public QWidget
{
  Q_OBJECT
public:
  SettingPanelMMGX(QWidget* parent, Engine* engine);
  ~SettingPanelMMGX() override = default;

  void reloadFont();
  void syncSettings();
  void checkHidden();
  std::vector<FT_Fixed>& mmgxCoords() { return currentValues_; }

signals:
  void mmgxCoordsChanged();

private:
  Engine* engine_;

  QCheckBox* showHiddenCheckBox_;
  QPushButton* resetDefaultButton_;
  QWidget* itemsListWidget_;
  UnboundScrollArea* scrollArea_;
  std::vector<MMGXSettingItem*> itemWidgets_;

  QVBoxLayout* mainLayout_;
  QVBoxLayout* listLayout_;
  QVBoxLayout* listWrapperLayout_;

  std::vector<FT_Fixed> currentValues_;
  std::vector<MMGXAxisInfo> currentAxes_;

  void createLayout();
  void createConnections();

  void itemChanged();
  void resetDefaultClicked();
};


class MMGXSettingItem
: public QFrame
{
  Q_OBJECT
public:
  MMGXSettingItem(QWidget* parent);
  ~MMGXSettingItem() override = default;

  void updateInfo(MMGXAxisInfo& info);
  FT_Fixed value() { return actualValue_; }
  void resetDefault();

signals:
  void valueChanged();

private:
  QLabel* nameLabel_;
  QLabel* valueLabel_;
  QSlider* slider_;

  QGridLayout* mainLayout_;

  FT_Fixed actualValue_;
  MMGXAxisInfo axisInfo_;

  void createLayout();
  void createConnections();
  void sliderValueChanged();
};


// end of settingpanelmmgx.hpp
