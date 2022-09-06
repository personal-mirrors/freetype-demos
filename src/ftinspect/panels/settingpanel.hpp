// settingpanel.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include "../engine/engine.hpp"
#include "../models/customcomboboxmodels.hpp"
#include "settingpanelmmgx.hpp"

#include <QWidget>
#include <QTabWidget>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QGridLayout>
#include <QBoxLayout>
#include <QPushButton>

class SettingPanel
: public QWidget
{
  Q_OBJECT
public:
  SettingPanel(QWidget* parent, Engine* engine, bool comparatorMode = false);
  ~SettingPanel() override = default;

  void onFontChanged();
  void applySettings();
  /*
   * When in comparator mode, this is needed to sync the hinting modes when
   * reloading the font.
   */
  void applyDelayedSettings();

  //////// Getters/Setters

  int antiAliasingModeIndex();
  bool kerningEnabled();
  bool lsbRsbDeltaEnabled();
  void setDefaultsPreset(int preset);

signals:
  void fontReloadNeeded();
  void repaintNeeded();

private:
  Engine* engine_;

  int currentCFFHintingMode_;
  int currentTTInterpreterVersion_;

  /*
   * There's two places where `SettingPanel` appears: On the left for most tabs,
   * and on the bottom in the comparator for each column. Therefore,
   * set `comparatorMode_` to `true` will change the panel for the Comparator
   * View.
   *
   * In comparator view, some updating is suppressed during GUI events.
   * Instead, updating was strictly passive called from the parent (comparator
   * view).
   */
  bool comparatorMode_ = false;
  bool debugMode_ = false;

  QTabWidget* tab_;

  QWidget* generalTab_;
  QWidget* hintingRenderingTab_;
  SettingPanelMMGX* mmgxPanel_;

  QLabel* gammaLabel_;
  QLabel* gammaValueLabel_;
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
  QCheckBox* stemDarkeningCheckBox_;
  QCheckBox* embeddedBitmapCheckBox_;
  QCheckBox* colorLayerCheckBox_;
  QCheckBox* kerningCheckBox_;
  QCheckBox* lsbRsbDeltaCheckBox_;

  AntiAliasingComboBoxModel* antiAliasingComboBoxModel_;
  HintingModeComboBoxModel* hintingModeComboBoxModel_;
  LCDFilterComboBoxModel* lcdFilterComboboxModel_;

  QComboBox* hintingModeComboBox_;
  QComboBox* antiAliasingComboBox_;
  QComboBox* lcdFilterComboBox_;
  QComboBox* paletteComboBox_;

  QSlider* gammaSlider_;

  QPushButton* backgroundButton_;
  QPushButton* foregroundButton_;
  QFrame* backgroundBlock_;
  QFrame* foregroundBlock_;

  QVBoxLayout* mainLayout_;
  QGridLayout* generalTabLayout_;
  QGridLayout* hintingRenderingTabLayout_;
  QVBoxLayout* debugLayout_;
  QHBoxLayout* gammaLayout_;
  QHBoxLayout* colorPickerLayout_;

  QColor backgroundColor_;
  QColor foregroundColor_;

  //////// Initializing funcs

  void createConnections();
  void createLayout();
  void createLayoutNormal();
  void createLayoutComparator();
  void setDefaults();

  //////// Other funcs

  void checkAllSettings();
  void checkHinting();
  void checkAutoHinting();
  void checkAntiAliasing();
  void checkPalette();

  void openBackgroundPicker();
  void openForegroundPicker();
  void updateGamma();
  void resetColorBlocks();
  void populatePalettes();
};


// end of settingpanel.hpp
