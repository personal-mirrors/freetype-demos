// settingpanelmmgx.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once

#include "../engine/mmgx.hpp"
#include "../widgets/customwidgets.hpp"

#include <QBoxLayout>
#include <QCheckBox>
#include <QDoubleValidator>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QToolButton>
#include <QWidget>

#include <freetype/fttypes.h>


class Engine;
class MMGXSettingItem;

class SettingPanelMMGX
: public QWidget
{
  Q_OBJECT

public:
  SettingPanelMMGX(QWidget* parent,
                   Engine* engine);
  ~SettingPanelMMGX() override = default;

  void reloadFont();
  void applySettings();
  void checkHidden();
  std::vector<FT_Fixed>& mmgxCoords() { return currentValues_; }

signals:
  void mmgxCoordsChanged();

private:
  Engine* engine_;

  QCheckBox* showHiddenCheckBox_;
  QCheckBox* groupingCheckBox_;
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

  void retrieveValues();
  void itemChanged(size_t index);
  void resetDefaultClicked();
  void checkGrouping();
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
  void setValue(FT_Fixed value);
  void resetDefault();

signals:
  void valueChanged();

private:
  QLabel* nameLabel_;
  QSlider* slider_;
  QPushButton* resetDefaultButton_;
  QLineEdit* valueLineEdit_;
  QDoubleValidator* valueValidator_;

  QGridLayout* mainLayout_;

  FT_Fixed actualValue_;
  MMGXAxisInfo axisInfo_;

  void createLayout();
  void createConnections();
  void sliderValueChanged();
  void lineEditChanged();
  void updateLineEdit();
  void updateSlider();
  void resetDefaultSingle();
};


// end of settingpanelmmgx.hpp
