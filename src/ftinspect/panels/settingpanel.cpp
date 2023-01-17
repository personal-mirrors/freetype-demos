// settingpanel.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "settingpanel.hpp"
#include "../uihelper.hpp"

#include <QColorDialog>

// For `FT_DEBUG_AUTOFIT`.
#include <freetype/config/ftoption.h>


SettingPanel::SettingPanel(QWidget* parent,
                           Engine* engine,
                           bool comparatorMode)
: QWidget(parent),
  engine_(engine),
  comparatorMode_(comparatorMode)
{
#ifdef FT_DEBUG_AUTOFIT
  debugMode_ = !comparatorMode_;
#else
  debugMode_ = false;
#endif
  createLayout();
  setDefaults();
  createConnections();
  checkAllSettings();
}


int
SettingPanel::antiAliasingModeIndex()
{
  return antiAliasingComboBox_->currentIndex();
}


bool
SettingPanel::kerningEnabled()
{
  return kerningCheckBox_->isChecked();
}


bool
SettingPanel::lsbRsbDeltaEnabled()
{
  return lsbRsbDeltaCheckBox_->isChecked();
}


void
SettingPanel::setDefaultsPreset(int preset)
{
  if (preset < 0)
    preset = 0;
  preset %= 3;
  switch (preset)
  {
  case 0:
    hintingCheckBox_->setChecked(true);
    autoHintingCheckBox_->setChecked(false);
    break;
  case 1:
    hintingCheckBox_->setChecked(true);
    autoHintingCheckBox_->setChecked(true);
    break;
  case 2:
    hintingCheckBox_->setChecked(false);
    autoHintingCheckBox_->setChecked(false);
    break;
  }
}


void
SettingPanel::checkAllSettings()
{
  onFontChanged();
  checkAntiAliasing();
}


void
SettingPanel::checkHinting()
{
  if (hintingCheckBox_->isChecked())
  {
    engine_->reloadFont();
    auto tricky = engine_->currentFontTricky();
    {
      QSignalBlocker blocker(autoHintingCheckBox_);
      autoHintingCheckBox_->setEnabled(!tricky);
      if (tricky)
        autoHintingCheckBox_->setChecked(false);
    }
    checkAutoHinting(); // This causes repainting.
  }
  else
  {
    hintingModeLabel_->setEnabled(false);
    hintingModeComboBox_->setEnabled(false);

    autoHintingCheckBox_->setEnabled(false);
    if (debugMode_)
    {
      horizontalHintingCheckBox_->setEnabled(false);
      verticalHintingCheckBox_->setEnabled(false);
      blueZoneHintingCheckBox_->setEnabled(false);
      segmentDrawingCheckBox_->setEnabled(false);
    }

    stemDarkeningCheckBox_->setEnabled(false);
    antiAliasingComboBoxModel_->setLightAntiAliasingEnabled(false);
    auto aaMode = antiAliasingComboBox_->currentIndex();
    if (aaMode == AntiAliasingComboBoxModel::AntiAliasing_Light
        || aaMode == AntiAliasingComboBoxModel::AntiAliasing_Light_SubPixel)
      antiAliasingComboBox_->setCurrentIndex(
        AntiAliasingComboBoxModel::AntiAliasing_Normal);

    emit repaintNeeded();
  }
}


void
SettingPanel::checkAutoHinting()
{
  if (autoHintingCheckBox_->isChecked())
  {
    hintingModeLabel_->setEnabled(false);
    hintingModeComboBox_->setEnabled(false);

    if (debugMode_)
    {
      horizontalHintingCheckBox_->setEnabled(true);
      verticalHintingCheckBox_->setEnabled(true);
      blueZoneHintingCheckBox_->setEnabled(true);
      segmentDrawingCheckBox_->setEnabled(true);
    }

    antiAliasingComboBoxModel_->setLightAntiAliasingEnabled(true);
    auto aaMode = antiAliasingComboBox_->currentIndex();
    stemDarkeningCheckBox_->setEnabled(
      aaMode == AntiAliasingComboBoxModel::AntiAliasing_Light
      || aaMode == AntiAliasingComboBoxModel::AntiAliasing_Light_SubPixel);
  }
  else
  {
    if (engine_->currentFontType() == Engine::FontType_CFF
        || engine_->currentFontType() == Engine::FontType_TrueType)
    {
      hintingModeLabel_->setEnabled(true);
      hintingModeComboBox_->setEnabled(true);
    }

    if (debugMode_)
    {
      horizontalHintingCheckBox_->setEnabled(false);
      verticalHintingCheckBox_->setEnabled(false);
      blueZoneHintingCheckBox_->setEnabled(false);
      segmentDrawingCheckBox_->setEnabled(false);
    }

    antiAliasingComboBoxModel_->setLightAntiAliasingEnabled(false);
    stemDarkeningCheckBox_->setEnabled(false);

    auto aaMode = antiAliasingComboBox_->currentIndex();
    if (aaMode == AntiAliasingComboBoxModel::AntiAliasing_Light
        || aaMode == AntiAliasingComboBoxModel::AntiAliasing_Light_SubPixel)
      antiAliasingComboBox_->setCurrentIndex(
          AntiAliasingComboBoxModel::AntiAliasing_Normal);
  }
  emit repaintNeeded();
}


void
SettingPanel::checkAntiAliasing()
{
  int index = antiAliasingComboBox_->currentIndex();
  auto isMono = index == AntiAliasingComboBoxModel::AntiAliasing_None;
  auto isLight
    = index == AntiAliasingComboBoxModel::AntiAliasing_Light
      || index == AntiAliasingComboBoxModel::AntiAliasing_Light_SubPixel;
  auto disableLCD
    = index == AntiAliasingComboBoxModel::AntiAliasing_None
      || index == AntiAliasingComboBoxModel::AntiAliasing::AntiAliasing_Normal
      || isLight;

  lcdFilterLabel_->setEnabled(!disableLCD);
  lcdFilterComboBox_->setEnabled(!disableLCD);
  stemDarkeningCheckBox_->setEnabled(isLight);
  gammaSlider_->setEnabled(!isMono);

  emit repaintNeeded();
}


void
SettingPanel::checkPalette()
{
  paletteComboBox_->setEnabled(colorLayerCheckBox_->isChecked()
                               && paletteComboBox_->count() > 0);
  engine_->resetCache();
  emit fontReloadNeeded();
}


void
SettingPanel::openBackgroundPicker()
{
  auto result = QColorDialog::getColor(backgroundColor_,
                                       this,
                                       tr("Background Color"));
  if (result.isValid())
  {
    backgroundColor_ = result;
    resetColorBlocks();
    emit repaintNeeded();
  }
}


void
SettingPanel::openForegroundPicker()
{
  auto result = QColorDialog::getColor(foregroundColor_,
                                       this,
                                       tr("Foreground Color"),
                                       QColorDialog::ShowAlphaChannel);
  if (result.isValid())
  {
    foregroundColor_ = result;
    resetColorBlocks();
    emit repaintNeeded();
  }
}


void
SettingPanel::updateGamma()
{
  gammaValueLabel_->setText(QString::number(gammaSlider_->value() / 10.0,
                            'f',
                            1));
  emit repaintNeeded();
}


void
SettingPanel::resetColorBlocks()
{
  foregroundBlock_->setStyleSheet(
    QString("QWidget {background-color: rgba(%1, %2, %3, %4);}")
      .arg(foregroundColor_.red())
      .arg(foregroundColor_.green())
      .arg(foregroundColor_.blue())
      .arg(foregroundColor_.alpha()));
  backgroundBlock_->setStyleSheet(
    QString("QWidget {background-color: rgba(%1, %2, %3, %4);}")
      .arg(backgroundColor_.red())
      .arg(backgroundColor_.green())
      .arg(backgroundColor_.blue())
      .arg(backgroundColor_.alpha()));
}


void
SettingPanel::populatePalettes()
{
  auto needToReload = false;
  auto& newPalettes = engine_->currentFontPalettes();
  auto newSize = static_cast<int>(newPalettes.size()); // This never exceeds!
  if (newSize != paletteComboBox_->count())
    needToReload = true;
  else
    for (int i = 0; i < newSize; ++i)
    {
      auto oldNameVariant = paletteComboBox_->itemData(i);
      if (!oldNameVariant.canConvert<QString>())
      {
        needToReload = true;
        break;
      }
      if (oldNameVariant.toString() != newPalettes[i].name)
      {
        needToReload = true;
        break;
      }
    }

  if (!needToReload)
    return;

  {
    QSignalBlocker blocker(paletteComboBox_);
    paletteComboBox_->clear();
    for (int i = 0; i < newSize; ++i)
    {
      auto str = QString("%1: %2").arg(i).arg(newPalettes[i].name);
      paletteComboBox_->addItem(str, newPalettes[i].name);
      paletteComboBox_->setItemData(i, str, Qt::ToolTipRole);
    }
  }

  emit fontReloadNeeded();
}


void
SettingPanel::onFontChanged()
{
  auto blockState = blockSignals(signalsBlocked() || comparatorMode_);

  engine_->reloadFont();
  if (engine_->currentFontType() == Engine::FontType_CFF)
  {
    hintingModeComboBoxModel_->setCurrentEngineType(
      HintingModeComboBoxModel::HintingEngineType_CFF, false);
    hintingModeComboBox_->setCurrentIndex(currentCFFHintingMode_);
  }
  else if (engine_->currentFontType() == Engine::FontType_TrueType)
  {
    auto tricky = engine_->currentFontTricky();
    hintingModeComboBoxModel_->setCurrentEngineType(
      HintingModeComboBoxModel::HintingEngineType_TrueType, tricky);
    hintingModeComboBox_->setCurrentIndex(
      tricky ? HintingModeComboBoxModel::HintingMode::HintingMode_TrueType_v35
             : currentTTInterpreterVersion_);
  }
  else
  {
    hintingModeLabel_->setEnabled(false);
    hintingModeComboBox_->setEnabled(false);
  }

  checkHinting();

  engine_->reloadFont();
  auto hasColor = engine_->currentFontHasColorLayers();
  colorLayerCheckBox_->setEnabled(hasColor);
  paletteComboBox_->setEnabled(colorLayerCheckBox_->isChecked()
                               && paletteComboBox_->count() > 0);
  populatePalettes();
  mmgxPanel_->reloadFont();
  blockSignals(blockState);

  // Place this after `blockSignals` to let the signals be emitted normally.
  auto bmapOnly = engine_->currentFontBitmapOnly();
  embeddedBitmapCheckBox_->setEnabled(
    !bmapOnly && engine_->currentFontHasEmbeddedBitmap());
  if (bmapOnly)
    embeddedBitmapCheckBox_->setChecked(true);
}


void
SettingPanel::applySettings()
{
  engine_->setLcdFilter(
    static_cast<FT_LcdFilter>(lcdFilterComboboxModel_->indexToValue(
      lcdFilterComboBox_->currentIndex())));

  auto aaSettings = antiAliasingComboBoxModel_->indexToValue(
    antiAliasingComboBox_->currentIndex());
  engine_->setAntiAliasingTarget(aaSettings.loadFlag);
  engine_->setRenderMode(aaSettings.renderMode);

  engine_->setAntiAliasingEnabled(antiAliasingComboBox_->currentIndex()
    != AntiAliasingComboBoxModel::AntiAliasing_None);
  engine_->setHinting(hintingCheckBox_->isChecked());
  engine_->setAutoHinting(autoHintingCheckBox_->isChecked());

  if (debugMode_)
  {
    engine_->setHorizontalHinting(horizontalHintingCheckBox_->isChecked());
    engine_->setVerticalHinting(verticalHintingCheckBox_->isChecked());
    engine_->setBlueZoneHinting(blueZoneHintingCheckBox_->isChecked());
    engine_->setShowSegments(segmentDrawingCheckBox_->isChecked());
  }

  engine_->renderingEngine()->setGamma(gammaSlider_->value() / 10.0);

  engine_->setEmbeddedBitmapEnabled(embeddedBitmapCheckBox_->isChecked());
  engine_->setPaletteIndex(paletteComboBox_->currentIndex());

  engine_->setUseColorLayer(colorLayerCheckBox_->isChecked());
  engine_->renderingEngine()->setLCDUsesBGR(aaSettings.isBGR);
  engine_->setLCDSubPixelPositioning(
    antiAliasingComboBox_->currentIndex()
      == AntiAliasingComboBoxModel::AntiAliasing_Light_SubPixel);

  engine_->renderingEngine()->setForeground(foregroundColor_.rgba());
  engine_->renderingEngine()->setBackground(backgroundColor_.rgba());
  mmgxPanel_->applySettings();
}


void
SettingPanel::applyDelayedSettings()
{
  // This must not be combined with `applySettings` since those engine
  // manipulations reset the whole cache!  Therefore, it must only be called
  // when the selection of the combobox actually changes (a.k.a.
  // `QComboBox::activate`).

  int index = hintingModeComboBox_->currentIndex();

  if (engine_->currentFontType() == Engine::FontType_CFF)
  {
    engine_->setCFFHintingMode(
      hintingModeComboBoxModel_->indexToCFFMode(index));
    if (index >= 0)
      currentCFFHintingMode_ = index;
  }
  else if (engine_->currentFontType() == Engine::FontType_TrueType)
  {
    engine_->setTTInterpreterVersion(
      hintingModeComboBoxModel_->indexToTTInterpreterVersion(index));
    if (index >= 0)
      currentTTInterpreterVersion_ = index;
  }

  engine_->setStemDarkening(stemDarkeningCheckBox_->isChecked());
}


void
SettingPanel::createLayout()
{
  hintingCheckBox_ = new QCheckBox(tr("Hinting"), this);

  hintingModeLabel_ = new QLabel(tr("Hinting Mode"), this);
  hintingModeLabel_->setAlignment(Qt::AlignRight);

  hintingModeComboBoxModel_ = new HintingModeComboBoxModel(this);
  hintingModeComboBox_ = new QComboBox(this);
  hintingModeComboBox_->setModel(hintingModeComboBoxModel_);
  hintingModeLabel_->setBuddy(hintingModeComboBox_);

  autoHintingCheckBox_ = new QCheckBox(tr("Auto-Hinting"), this);
  stemDarkeningCheckBox_ = new QCheckBox(tr("Stem Darkening"), this);

  if (debugMode_)
  {
    horizontalHintingCheckBox_ = new QCheckBox(tr("Horizontal Hinting"), this);
    verticalHintingCheckBox_ = new QCheckBox(tr("Vertical Hinting"), this);
    blueZoneHintingCheckBox_ = new QCheckBox(tr("Blue-Zone Hinting"), this);
    segmentDrawingCheckBox_ = new QCheckBox(tr("Segment Drawing"), this);
  }

  embeddedBitmapCheckBox_ = new QCheckBox(tr("Enable Embedded Bitmap"), this);
  colorLayerCheckBox_ = new QCheckBox(tr("Enable Color Layer"), this);

  if (comparatorMode_)
  {
    kerningCheckBox_ = new QCheckBox(tr("Kerning"), this);
    lsbRsbDeltaCheckBox_ = new QCheckBox(tr("LSB/RSB Delta"), this);
  }

  antiAliasingLabel_ = new QLabel(tr("Anti-Aliasing"), this);
  antiAliasingLabel_->setAlignment(Qt::AlignRight);

  antiAliasingComboBoxModel_ = new AntiAliasingComboBoxModel(this);
  antiAliasingComboBox_ = new QComboBox(this);
  antiAliasingComboBox_->setModel(antiAliasingComboBoxModel_);
  antiAliasingLabel_->setBuddy(antiAliasingComboBox_);

  lcdFilterLabel_ = new QLabel(tr("LCD Filter"), this);
  lcdFilterLabel_->setAlignment(Qt::AlignRight);

  lcdFilterComboboxModel_ = new LCDFilterComboBoxModel(this);
  lcdFilterComboBox_ = new QComboBox(this);
  lcdFilterComboBox_->setModel(lcdFilterComboboxModel_);
  lcdFilterLabel_->setBuddy(lcdFilterComboBox_);

  paletteLabel_ = new QLabel(tr("Palette: "), this);

  paletteComboBox_ = new QComboBox(this);
  paletteLabel_->setBuddy(paletteComboBox_);

  gammaLabel_ = new QLabel(tr("Gamma"), this);
  gammaLabel_->setAlignment(Qt::AlignRight);
  gammaSlider_ = new QSlider(Qt::Horizontal, this);
  gammaSlider_->setRange(3, 30); // in 1/10
  gammaSlider_->setTickPosition(QSlider::TicksBelow);
  gammaSlider_->setTickInterval(5);
  gammaSlider_->setPageStep(1);
  gammaSlider_->setSingleStep(1);
  gammaLabel_->setBuddy(gammaSlider_);
  gammaValueLabel_ = new QLabel(this);

  mmgxPanel_ = new SettingPanelMMGX(this, engine_);

  backgroundButton_ = new QPushButton(tr("Background"), this);
  foregroundButton_ = new QPushButton(tr("Foreground"), this);

  backgroundBlock_ = new QFrame(this);
  backgroundBlock_->setFrameStyle(QFrame::Box);
  backgroundBlock_->setLineWidth(1);
  backgroundBlock_->setFixedWidth(18);

  foregroundBlock_ = new QFrame(this);
  foregroundBlock_->setFrameStyle(QFrame::Box);
  foregroundBlock_->setLineWidth(1);
  foregroundBlock_->setFixedWidth(18);

  generalTab_ = new QWidget(this);

  generalTab_->setSizePolicy(QSizePolicy::MinimumExpanding,
                             QSizePolicy::MinimumExpanding);

  tab_ = new QTabWidget(this);
  tab_->setSizePolicy(QSizePolicy::MinimumExpanding,
                      QSizePolicy::MinimumExpanding);

  // Tooltips
  hintingCheckBox_->setToolTip(tr("Enable hinting a.k.a. grid-fitting."));
  hintingModeComboBox_->setToolTip(tr(
    "Modes not available for current font type will be disabled."
    " No effect when auto-hinting is enabled"));
  autoHintingCheckBox_->setToolTip(tr("Enable FreeType Auto-Hinter."));
  if (debugMode_)
  {
    horizontalHintingCheckBox_->setToolTip(tr("(auto-hinter debug option)"));
    verticalHintingCheckBox_->setToolTip(tr("(auto-hinter debug option)"));
    blueZoneHintingCheckBox_->setToolTip(tr("(auto-hinter debug option)"));
    segmentDrawingCheckBox_->setToolTip(tr("(auto-hinter debug option)"));
  }
  antiAliasingComboBox_->setToolTip(tr("Select anti-aliasing mode."));
  lcdFilterComboBox_->setToolTip(tr(
    "Select LCD filter (only valid when LCD AA is enabled)."));
  embeddedBitmapCheckBox_->setToolTip(tr(
    "Enable embedded bitmap strikes (force-enabled for bitmap-only fonts)."));
  stemDarkeningCheckBox_->setToolTip(tr(
    "Enable stem darkening (only valid for auto-hinter with gamma"
    " correction enabled and with Light AA modes)."));
  gammaSlider_->setToolTip("Gamma correction value.");
  colorLayerCheckBox_->setToolTip(tr("Enable color layer rendering."));
  paletteComboBox_->setToolTip(tr(
    "Select color layer palette (only valid when"
    " any palette exists in the font)."));
  if (comparatorMode_)
  {
    kerningCheckBox_->setToolTip(tr(
      "Enable kerning (GPOS table not supported)."));
    lsbRsbDeltaCheckBox_->setToolTip(tr(
      "Enable LSB/RSB delta positioning (only valid when hinting is"
      " enabled)."));
  }
  backgroundButton_->setToolTip(tr("Set canvas background color."));
  foregroundButton_->setToolTip(tr("Set text color."));

  // Layouting
  if (debugMode_)
  {
    debugLayout_ = new QVBoxLayout;
    debugLayout_->setContentsMargins(20, 0, 0, 0);
    debugLayout_->addWidget(horizontalHintingCheckBox_);
    debugLayout_->addWidget(verticalHintingCheckBox_);
    debugLayout_->addWidget(blueZoneHintingCheckBox_);
    debugLayout_->addWidget(segmentDrawingCheckBox_);
  }

  gammaLayout_ = new QHBoxLayout;
  gammaLayout_->addWidget(gammaLabel_);
  gammaLayout_->addWidget(gammaSlider_);
  gammaLayout_->addWidget(gammaValueLabel_);

  colorPickerLayout_ = new QHBoxLayout;
  colorPickerLayout_->addWidget(backgroundBlock_);
  colorPickerLayout_->addWidget(backgroundButton_, 1);
  colorPickerLayout_->addWidget(foregroundButton_, 1);
  colorPickerLayout_->addWidget(foregroundBlock_);

  if (comparatorMode_)
    createLayoutComparator();
  else
    createLayoutNormal();

  mainLayout_ = new QVBoxLayout;
  mainLayout_->addWidget(tab_);
  setLayout(mainLayout_);
  mainLayout_->setContentsMargins(0, 0, 0, 0);
  setContentsMargins(0, 0, 0, 0);
}


void
SettingPanel::createLayoutNormal()
{
  generalTabLayout_ = new QGridLayout;

  gridLayout2ColAddWidget(generalTabLayout_, hintingCheckBox_);
  gridLayout2ColAddWidget(generalTabLayout_,
                          hintingModeLabel_, hintingModeComboBox_);
  gridLayout2ColAddWidget(generalTabLayout_, autoHintingCheckBox_);

  if (debugMode_)
    gridLayout2ColAddLayout(generalTabLayout_, debugLayout_);

  gridLayout2ColAddItem(generalTabLayout_,
                        new QSpacerItem(0, 20, QSizePolicy::Minimum,
                                        QSizePolicy::MinimumExpanding));

  gridLayout2ColAddWidget(generalTabLayout_,
                          antiAliasingLabel_, antiAliasingComboBox_);
  gridLayout2ColAddWidget(generalTabLayout_,
                          lcdFilterLabel_, lcdFilterComboBox_);

  gridLayout2ColAddItem(generalTabLayout_,
                        new QSpacerItem(0, 20, QSizePolicy::Minimum,
                                        QSizePolicy::MinimumExpanding));

  gridLayout2ColAddLayout(generalTabLayout_, colorPickerLayout_);
  gridLayout2ColAddLayout(generalTabLayout_, gammaLayout_);
  gridLayout2ColAddWidget(generalTabLayout_, stemDarkeningCheckBox_);
  gridLayout2ColAddWidget(generalTabLayout_, embeddedBitmapCheckBox_);
  gridLayout2ColAddWidget(generalTabLayout_, colorLayerCheckBox_);
  gridLayout2ColAddWidget(generalTabLayout_,
                          paletteLabel_, paletteComboBox_);

  gridLayout2ColAddItem(generalTabLayout_,
                        new QSpacerItem(0, 20, QSizePolicy::Minimum,
                                        QSizePolicy::MinimumExpanding));

  generalTabLayout_->setColumnStretch(1, 1);
  generalTab_->setLayout(generalTabLayout_);

  tab_->addTab(generalTab_, tr("General"));
  tab_->addTab(mmgxPanel_, tr("MM/GX"));

  tab_->setTabToolTip(0, tr("General settings."));
  tab_->setTabToolTip(1, tr("MM/GX axis parameters."));
}


void
SettingPanel::createLayoutComparator()
{
  hintingRenderingTab_ = new QWidget(this);

  generalTabLayout_ = new QGridLayout;
  hintingRenderingTabLayout_ = new QGridLayout;

  // Hinting & Rendering
  gridLayout2ColAddWidget(hintingRenderingTabLayout_, hintingCheckBox_);
  gridLayout2ColAddWidget(hintingRenderingTabLayout_,
                          hintingModeLabel_, hintingModeComboBox_);
  gridLayout2ColAddWidget(hintingRenderingTabLayout_, autoHintingCheckBox_);

  if (debugMode_)
    gridLayout2ColAddLayout(hintingRenderingTabLayout_, debugLayout_);

  gridLayout2ColAddWidget(hintingRenderingTabLayout_,
                          antiAliasingLabel_, antiAliasingComboBox_);
  gridLayout2ColAddWidget(hintingRenderingTabLayout_,
                          lcdFilterLabel_, lcdFilterComboBox_);

  gridLayout2ColAddLayout(hintingRenderingTabLayout_, gammaLayout_);
  gridLayout2ColAddWidget(hintingRenderingTabLayout_, stemDarkeningCheckBox_);

  // General
  gridLayout2ColAddLayout(generalTabLayout_, colorPickerLayout_);
  gridLayout2ColAddWidget(generalTabLayout_, embeddedBitmapCheckBox_);
  gridLayout2ColAddWidget(generalTabLayout_, colorLayerCheckBox_);
  gridLayout2ColAddWidget(generalTabLayout_,
                          paletteLabel_, paletteComboBox_);

  gridLayout2ColAddWidget(generalTabLayout_, kerningCheckBox_);
  gridLayout2ColAddWidget(generalTabLayout_, lsbRsbDeltaCheckBox_);

  generalTabLayout_->setColumnStretch(1, 1);
  hintingRenderingTabLayout_->setColumnStretch(1, 1);
  generalTab_->setLayout(generalTabLayout_);
  hintingRenderingTab_->setLayout(hintingRenderingTabLayout_);

  tab_->addTab(hintingRenderingTab_, tr("Hinting && Rendering"));
  tab_->addTab(generalTab_, tr("General"));
  tab_->addTab(mmgxPanel_, tr("MM/GX"));
  tab_->setTabToolTip(0, tr("Settings about hinting and rendering."));
  tab_->setTabToolTip(1, tr("General settings."));
  tab_->setTabToolTip(2, tr("MM/GX axis parameters."));
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
}


void
SettingPanel::createConnections()
{
  // Use `QOverload` here to prevent ambiguities.
  connect(hintingModeComboBox_,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &SettingPanel::fontReloadNeeded);
  connect(antiAliasingComboBox_,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &SettingPanel::checkAntiAliasing);
  connect(lcdFilterComboBox_,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &SettingPanel::repaintNeeded);
  connect(paletteComboBox_,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &SettingPanel::checkPalette);

  connect(gammaSlider_, &QSlider::valueChanged,
          this, &SettingPanel::updateGamma);

  connect(hintingCheckBox_, &QCheckBox::clicked,
          this, &SettingPanel::checkHinting);

  if (debugMode_)
  {
    connect(horizontalHintingCheckBox_, &QCheckBox::clicked,
            this, &SettingPanel::repaintNeeded);
    connect(verticalHintingCheckBox_, &QCheckBox::clicked,
            this, &SettingPanel::repaintNeeded);
    connect(blueZoneHintingCheckBox_, &QCheckBox::clicked,
            this, &SettingPanel::repaintNeeded);
    connect(segmentDrawingCheckBox_, &QCheckBox::clicked,
            this, &SettingPanel::repaintNeeded);
  }

  connect(autoHintingCheckBox_, &QCheckBox::clicked,
          this, &SettingPanel::checkAutoHinting);
  connect(embeddedBitmapCheckBox_, &QCheckBox::clicked,
          this, &SettingPanel::fontReloadNeeded);
  connect(stemDarkeningCheckBox_, &QCheckBox::clicked,
          this, &SettingPanel::fontReloadNeeded);
  connect(colorLayerCheckBox_, &QCheckBox::clicked,
          this, &SettingPanel::checkPalette);

  if (comparatorMode_)
  {
    connect(kerningCheckBox_, &QCheckBox::clicked,
            this, &SettingPanel::repaintNeeded);
    connect(lsbRsbDeltaCheckBox_, &QCheckBox::clicked,
            this, &SettingPanel::repaintNeeded);
  }

  connect(backgroundButton_, &QPushButton::clicked,
          this, &SettingPanel::openBackgroundPicker);
  connect(foregroundButton_, &QPushButton::clicked,
          this, &SettingPanel::openForegroundPicker);

  connect(mmgxPanel_, &SettingPanelMMGX::mmgxCoordsChanged,
          this, &SettingPanel::fontReloadNeeded);
}


void
SettingPanel::setDefaults()
{
  Engine::EngineDefaultValues& defaults = engine_->engineDefaults();

  hintingModeComboBoxModel_->setSupportedModes(
    { defaults.ttInterpreterVersionDefault,
      defaults.ttInterpreterVersionOther,
      defaults.ttInterpreterVersionOther1 },
    { defaults.cffHintingEngineDefault,
      defaults.cffHintingEngineOther });

  currentCFFHintingMode_
    = hintingModeComboBoxModel_->cffModeToIndex(
                                   defaults.cffHintingEngineDefault);
  currentTTInterpreterVersion_
    = hintingModeComboBoxModel_->ttInterpreterVersionToIndex(
                                   defaults.ttInterpreterVersionDefault);

  hintingCheckBox_->setChecked(true);

  antiAliasingComboBox_->setCurrentIndex(
    AntiAliasingComboBoxModel::AntiAliasing_Normal);
  lcdFilterComboBox_->setCurrentIndex(
    LCDFilterComboBoxModel::LCDFilter_Light);

  if (debugMode_)
  {
    horizontalHintingCheckBox_->setChecked(true);
    verticalHintingCheckBox_->setChecked(true);
    blueZoneHintingCheckBox_->setChecked(true);
    segmentDrawingCheckBox_->setChecked(false);
  }

  embeddedBitmapCheckBox_->setChecked(false);
  colorLayerCheckBox_->setChecked(true);
  paletteComboBox_->setEnabled(false);

  if (comparatorMode_)
  {
    kerningCheckBox_->setChecked(true);
    lsbRsbDeltaCheckBox_->setChecked(true);
  }

  // These need to be set even in Comperator mode.
  backgroundColor_ = Qt::white;
  foregroundColor_ = Qt::black;
  resetColorBlocks();

  gammaSlider_->setValue(18); // 1.8
  updateGamma();
}


// end of settingpanel.cpp
