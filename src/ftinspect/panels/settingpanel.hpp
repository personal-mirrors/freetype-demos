// settingpanel.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include "../engine/engine.hpp"
#include "../models/customcomboboxmodels.hpp"

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
  SettingPanel(QWidget* parent, Engine* engine);
  ~SettingPanel() override = default;

  void syncSettings();

  //////// Getters/Setters

  int antiAliasingModeIndex();

signals:
  void fontReloadNeeded();
  void repaintNeeded();

  //////// `checkXXX` funcs

public slots:
  void checkAllSettings();
  void onFontChanged();
  void checkHintingMode();
  void checkAutoHinting();
  void checkAntiAliasing();
  void checkPalette();

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
  QLabel* paletteLabel_;

  QCheckBox* hintingCheckBox_;
  QCheckBox* horizontalHintingCheckBox_;
  QCheckBox* verticalHintingCheckBox_;
  QCheckBox* blueZoneHintingCheckBox_;
  QCheckBox* segmentDrawingCheckBox_;
  QCheckBox* autoHintingCheckBox_;
  QCheckBox* embeddedBitmapCheckBox_;
  QCheckBox* colorLayerCheckBox_;

  AntiAliasingComboBoxModel* antiAliasingComboBoxModel_;
  HintingModeComboBoxModel* hintingModeComboBoxModel_;
  LCDFilterComboBoxModel* lcdFilterComboboxModel_;

  QComboBox* hintingModeComboBox_;
  QComboBox* antiAliasingComboBox_;
  QComboBox* lcdFilterComboBox_;
  QComboBox* paletteComboBox_;

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
  QHBoxLayout* paletteLayout_;

  QVBoxLayout* generalTabLayout_;

  //////// Initializing funcs

  void createConnections();
  void createLayout();
  void setDefaults();

  void populatePalettes();
};


// end of settingpanel.hpp
