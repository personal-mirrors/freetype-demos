// settingpanel.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "settingpanel.hpp"

#include "../uihelper.hpp"

SettingPanel::SettingPanel(QWidget* parent,
                           Engine* engine,
                           bool comparatorMode)
: QWidget(parent),
  engine_(engine),
  comparatorMode_(comparatorMode)
{
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
SettingPanel::checkAllSettings()
{
  onFontChanged();
  checkAutoHinting();
  checkAntiAliasing();
}


void
SettingPanel::onFontChanged()
{
  auto blockState = blockSignals(signalsBlocked() || comparatorMode_);
  if (hintingCheckBox_->isChecked())
  {
    if (engine_->currentFontType() == Engine::FontType_CFF)
    {
      hintingModeComboBoxModel_->setCurrentEngineType(
        HintingModeComboBoxModel::HintingEngineType_CFF);
      hintingModeComboBox_->setCurrentIndex(currentCFFHintingMode_);
    }
    else if (engine_->currentFontType() == Engine::FontType_TrueType)
    {
      hintingModeComboBoxModel_->setCurrentEngineType(
        HintingModeComboBoxModel::HintingEngineType_TrueType);
      hintingModeComboBox_->setCurrentIndex(currentTTInterpreterVersion_);
    }
    else
    {
      hintingModeLabel_->setEnabled(false);
      hintingModeComboBox_->setEnabled(false);
    }

    autoHintingCheckBox_->setEnabled(true);
    checkAutoHinting(); // this will emit repaint
  }
  else
  {
    hintingModeLabel_->setEnabled(false);
    hintingModeComboBox_->setEnabled(false);

    autoHintingCheckBox_->setEnabled(false);
    horizontalHintingCheckBox_->setEnabled(false);
    verticalHintingCheckBox_->setEnabled(false);
    blueZoneHintingCheckBox_->setEnabled(false);
    segmentDrawingCheckBox_->setEnabled(false);

    antiAliasingComboBoxModel_->setLightAntiAliasingEnabled(false);
    if (antiAliasingComboBox_->currentIndex()
      == AntiAliasingComboBoxModel::AntiAliasing_Light)
      antiAliasingComboBox_->setCurrentIndex(
        AntiAliasingComboBoxModel::AntiAliasing_Normal);
    
    emit repaintNeeded();
  }

  populatePalettes();
  blockSignals(blockState);
}


void
SettingPanel::populatePalettes()
{
  auto needToReload = false;
  auto& newPalettes = engine_->currentFontPalettes();
  auto newSize = static_cast<int>(newPalettes.size()); // this never exceeds!
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
      paletteComboBox_->addItem(
        QString("%1: %2")
          .arg(i)
          .arg(newPalettes[i].name),
        newPalettes[i].name);
  }

  emit fontReloadNeeded();
}


void
SettingPanel::checkHintingMode()
{
  if (!comparatorMode_)
    applyHintingMode();

  emit fontReloadNeeded();
}


void
SettingPanel::applyHintingMode()
{
  // This must not be combined into `syncSettings`:
  // those engine manipulations will reset the whole cache!!
  // Therefore must only be called when the selection of the combo box actually
  // changes a.k.a. QComboBox::activate.

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
}


void
SettingPanel::checkAutoHinting()
{
  if (autoHintingCheckBox_->isChecked())
  {
    hintingModeLabel_->setEnabled(false);
    hintingModeComboBox_->setEnabled(false);

    horizontalHintingCheckBox_->setEnabled(true);
    verticalHintingCheckBox_->setEnabled(true);
    blueZoneHintingCheckBox_->setEnabled(true);
    segmentDrawingCheckBox_->setEnabled(true);

    antiAliasingComboBoxModel_->setLightAntiAliasingEnabled(true);
  }
  else
  {
    if (engine_->currentFontType() == Engine::FontType_CFF
        || engine_->currentFontType() == Engine::FontType_TrueType)
    {
      hintingModeLabel_->setEnabled(true);
      hintingModeComboBox_->setEnabled(true);
    }

    horizontalHintingCheckBox_->setEnabled(false);
    verticalHintingCheckBox_->setEnabled(false);
    blueZoneHintingCheckBox_->setEnabled(false);
    segmentDrawingCheckBox_->setEnabled(false);

    antiAliasingComboBoxModel_->setLightAntiAliasingEnabled(false);

    if (antiAliasingComboBox_->currentIndex() 
      == AntiAliasingComboBoxModel::AntiAliasing_Light)
      antiAliasingComboBox_->setCurrentIndex(
        AntiAliasingComboBoxModel::AntiAliasing_Normal);
  }
  emit repaintNeeded();
}


void
SettingPanel::checkAntiAliasing()
{
  int index = antiAliasingComboBox_->currentIndex();

  if (index == AntiAliasingComboBoxModel::AntiAliasing_None
      || index == AntiAliasingComboBoxModel::AntiAliasing::AntiAliasing_Normal
      || index == AntiAliasingComboBoxModel::AntiAliasing_Light)
  {
    lcdFilterLabel_->setEnabled(false);
    lcdFilterComboBox_->setEnabled(false);
  }
  else
  {
    lcdFilterLabel_->setEnabled(true);
    lcdFilterComboBox_->setEnabled(true);
  }
  emit repaintNeeded();
}


void
SettingPanel::checkPalette()
{
  paletteComboBox_->setEnabled(colorLayerCheckBox_->isChecked());
  emit repaintNeeded();
}


void
SettingPanel::syncSettings()
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
  engine_->setHorizontalHinting(horizontalHintingCheckBox_->isChecked());
  engine_->setVerticalHinting(verticalHintingCheckBox_->isChecked());
  engine_->setBlueZoneHinting(blueZoneHintingCheckBox_->isChecked());
  engine_->setShowSegments(segmentDrawingCheckBox_->isChecked());

  engine_->setGamma(gammaSlider_->value());

  engine_->setEmbeddedBitmap(embeddedBitmapCheckBox_->isChecked());
  engine_->setPaletteIndex(paletteComboBox_->currentIndex());

  engine_->setUseColorLayer(colorLayerCheckBox_->isChecked());
  engine_->setLCDUsesBGR(aaSettings.isBGR);
  engine_->setLCDSubPixelPositioning(
    antiAliasingComboBox_->currentIndex()
      == AntiAliasingComboBoxModel::AntiAliasing_Light_SubPixel);
}


void
SettingPanel::createConnections()
{
  // use `qOverload` here to prevent ambiguity.
  connect(hintingModeComboBox_, 
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &SettingPanel::checkHintingMode);
  connect(antiAliasingComboBox_,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &SettingPanel::checkAntiAliasing);
  connect(lcdFilterComboBox_, 
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &SettingPanel::repaintNeeded);
  connect(paletteComboBox_,
          QOverload<int>::of(&QComboBox::currentIndexChanged), 
          this, &SettingPanel::repaintNeeded);

  connect(gammaSlider_, &QSlider::valueChanged,
          this, &SettingPanel::repaintNeeded);
  
  connect(hintingCheckBox_, &QCheckBox::clicked,
          this, &SettingPanel::repaintNeeded);

  if (!comparatorMode_)
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
  connect(colorLayerCheckBox_, &QCheckBox::clicked,
          this, &SettingPanel::checkPalette);

  if (comparatorMode_)
  {
    connect(kerningCheckBox_, &QCheckBox::clicked,
            this, &SettingPanel::repaintNeeded);
    connect(lsbRsbDeltaCheckBox_, &QCheckBox::clicked,
            this, &SettingPanel::repaintNeeded);
  }
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
  horizontalHintingCheckBox_ = new QCheckBox(tr("Horizontal Hinting"), this);
  verticalHintingCheckBox_ = new QCheckBox(tr("Vertical Hinting"), this);
  blueZoneHintingCheckBox_ = new QCheckBox(tr("Blue-Zone Hinting"), this);
  segmentDrawingCheckBox_ = new QCheckBox(tr("Segment Drawing"), this);
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

  int width;
  // make all labels have the same width
  width = hintingModeLabel_->minimumSizeHint().width();
  width = qMax(antiAliasingLabel_->minimumSizeHint().width(), width);
  width = qMax(lcdFilterLabel_->minimumSizeHint().width(), width);
  hintingModeLabel_->setMinimumWidth(width);
  antiAliasingLabel_->setMinimumWidth(width);
  lcdFilterLabel_->setMinimumWidth(width);

  // ensure that all items in combo boxes fit completely;
  // also make all combo boxes have the same width
  width = hintingModeComboBox_->minimumSizeHint().width();
  width = qMax(antiAliasingComboBox_->minimumSizeHint().width(), width);
  width = qMax(lcdFilterComboBox_->minimumSizeHint().width(), width);
  hintingModeComboBox_->setMinimumWidth(width);
  antiAliasingComboBox_->setMinimumWidth(width);
  lcdFilterComboBox_->setMinimumWidth(width);

  gammaLabel_ = new QLabel(tr("Gamma"), this);
  gammaLabel_->setAlignment(Qt::AlignRight);
  gammaSlider_ = new QSlider(Qt::Horizontal, this);
  gammaSlider_->setRange(0, 30); // in 1/10th
  gammaSlider_->setTickPosition(QSlider::TicksBelow);
  gammaSlider_->setTickInterval(5);
  gammaLabel_->setBuddy(gammaSlider_);

  debugLayout_ = new QVBoxLayout;
  debugLayout_->setContentsMargins(20, 0, 0, 0);
  debugLayout_->addWidget(horizontalHintingCheckBox_);
  debugLayout_->addWidget(verticalHintingCheckBox_);
  debugLayout_->addWidget(blueZoneHintingCheckBox_);
  debugLayout_->addWidget(segmentDrawingCheckBox_);

  gammaLayout_ = new QHBoxLayout;
  gammaLayout_->addWidget(gammaLabel_);
  gammaLayout_->addWidget(gammaSlider_);

  generalTabLayout_ = new QGridLayout;

  gridLayout2ColAddWidget(generalTabLayout_, hintingCheckBox_);
  gridLayout2ColAddWidget(generalTabLayout_, 
                          hintingModeLabel_, hintingModeComboBox_);
  gridLayout2ColAddWidget(generalTabLayout_, autoHintingCheckBox_);

  if (!comparatorMode_)
    gridLayout2ColAddLayout(generalTabLayout_, debugLayout_);

  if (!comparatorMode_)
    gridLayout2ColAddItem(generalTabLayout_,
                          new QSpacerItem(0, 20, QSizePolicy::Minimum,
                                          QSizePolicy::MinimumExpanding));

  gridLayout2ColAddWidget(generalTabLayout_, 
                          antiAliasingLabel_, antiAliasingComboBox_);
  gridLayout2ColAddWidget(generalTabLayout_, 
                          lcdFilterLabel_, lcdFilterComboBox_);

  if (!comparatorMode_)
    gridLayout2ColAddItem(generalTabLayout_,
                          new QSpacerItem(0, 20, QSizePolicy::Minimum,
                                          QSizePolicy::MinimumExpanding));

  gridLayout2ColAddLayout(generalTabLayout_, gammaLayout_);
  gridLayout2ColAddWidget(generalTabLayout_, embeddedBitmapCheckBox_);
  gridLayout2ColAddWidget(generalTabLayout_, colorLayerCheckBox_);
  gridLayout2ColAddWidget(generalTabLayout_, 
                          paletteLabel_, paletteComboBox_);

  if (comparatorMode_)
  {
    gridLayout2ColAddWidget(generalTabLayout_, kerningCheckBox_);
    gridLayout2ColAddWidget(generalTabLayout_, lsbRsbDeltaCheckBox_);
  }

  if (!comparatorMode_)
    gridLayout2ColAddItem(generalTabLayout_,
                          new QSpacerItem(0, 20, QSizePolicy::Minimum,
                                          QSizePolicy::MinimumExpanding));

  generalTabLayout_->setColumnStretch(1, 1);

  generalTab_ = new QWidget(this);
  generalTab_->setLayout(generalTabLayout_);
  generalTab_->setSizePolicy(QSizePolicy::MinimumExpanding,
                             QSizePolicy::MinimumExpanding);

  mmgxTab_ = new QWidget(this);

  tab_ = new QTabWidget(this);
  tab_->addTab(generalTab_, tr("General"));
  tab_->addTab(mmgxTab_, tr("MM/GX"));
  tab_->setSizePolicy(QSizePolicy::MinimumExpanding,
                      QSizePolicy::MinimumExpanding);

  mainLayout_ = new QVBoxLayout;
  mainLayout_->addWidget(tab_);
  setLayout(mainLayout_);
  mainLayout_->setContentsMargins(0, 0, 0, 0);
  setContentsMargins(0, 0, 0, 0);

  if (comparatorMode_)
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
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

  horizontalHintingCheckBox_->setChecked(true);
  verticalHintingCheckBox_->setChecked(true);
  blueZoneHintingCheckBox_->setChecked(true);
  embeddedBitmapCheckBox_->setChecked(false);
  colorLayerCheckBox_->setChecked(true);

  if (comparatorMode_)
  {
    kerningCheckBox_->setChecked(true);
    lsbRsbDeltaCheckBox_->setChecked(true);
  }

  gammaSlider_->setValue(18); // 1.8
}


// end of settingpanel.cpp
