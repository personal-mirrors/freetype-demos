// settingpanel.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include "../engine/engine.hpp"
#include "../models/ttsettingscomboboxmodel.hpp"

#include <QWidget>
#include <QTabWidget>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QBoxLayout>

class SettingPanel
: public QWidget
{
  Q_OBJECT
public:
  SettingPanel(Engine* engine);
  ~SettingPanel() = default;

  void syncSettings();

  //////// Getters/Setters

  int antiAliasingModeIndex();

  // TODO This would eventually go to separate panel for ftglyph (Singular View)
  bool showBitmapChecked();
  bool showOutLinesChecked();
  bool showPointNumbersChecked();
  bool showPointsChecked();

signals:
  void fontReloadNeeded();
  void repaintNeeded();

  //////// `checkXXX` funcs

public slots:
  void checkAllSettings();
  void checkHinting();
  void checkHintingMode();
  void checkAutoHinting();
  void checkAntiAliasing();
  void checkShowPoints();
  void checkLCDFilter();

private:
  Engine* engine_;

  int currentCFFHintingMode_;
  int currentTTInterpreterVersion_;

  QTabWidget* tab_;

  QWidget* generalTab_;
  QWidget* mmgxTab_;

  QLabel* gammaLabel_;
  QLabel* antiAliasingLabel_;
  QLabel* hintingModeLabel_;
  QLabel* lcdFilterLabel_;

  QCheckBox* hintingCheckBox_;
  QCheckBox* horizontalHintingCheckBox_;
  QCheckBox* verticalHintingCheckBox_;
  QCheckBox* blueZoneHintingCheckBox_;
  QCheckBox* segmentDrawingCheckBox_;
  QCheckBox* autoHintingCheckBox_;
  QCheckBox* showBitmapCheckBox_;
  QCheckBox* showOutlinesCheckBox_;
  QCheckBox* showPointNumbersCheckBox_;
  QCheckBox* showPointsCheckBox_;

  AntiAliasingComboBoxModel* antiAliasingComboBoxModel_;
  HintingModeComboBoxModel* hintingModeComboBoxModel_;
  LCDFilterComboBoxModel* lcdFilterComboboxModel_;

  QComboBox* hintingModeComboBox_;
  QComboBox* antiAliasingComboBox_;
  QComboBox* lcdFilterComboBox_;

  QSlider* gammaSlider_;

  QVBoxLayout* mainLayout_;
  QHBoxLayout* hintingModeLayout_;
  QHBoxLayout* horizontalHintingLayout_;
  QHBoxLayout* verticalHintingLayout_;
  QHBoxLayout* blueZoneHintingLayout_;
  QHBoxLayout* segmentDrawingLayout_;
  QHBoxLayout* antiAliasingLayout_;
  QHBoxLayout* lcdFilterLayout_;
  QHBoxLayout* gammaLayout_;
  QHBoxLayout* pointNumbersLayout_;

  QVBoxLayout* generalTabLayout_;

  //////// Initializing funcs

  void createConnections();
  void createLayout();
  void setDefaults();
};


// end of settingpanel.hpp
