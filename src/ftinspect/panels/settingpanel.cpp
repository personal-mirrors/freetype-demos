// settingpanel.cpp

// Copyright (C) 2022 by Charlie Jiang.


#include "settingpanel.hpp"

SettingPanel::SettingPanel(QWidget* parent, Engine* engine)
: QWidget(parent), engine_(engine)
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
}


void
SettingPanel::populatePalettes()
{
  auto needToReload = false;
  auto& newPalettes = engine_->currentFontPalettes();
  if (newPalettes.size() != (size_t)paletteComboBox_->count())
    needToReload = true;
  else
    for (size_t i = 0; i < newPalettes.size(); ++i)
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
    for (size_t i = 0; i < newPalettes.size(); ++i)
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
  // This must not be combined into `syncSettings`:
  // those engine manipulations will reset the whole cache!!
  // Therefore must only be called when the selection of the combo box actually
  // changes a.k.a. QComboBox::activate.
  int index = hintingModeComboBox_->currentIndex();

  if (engine_->currentFontType() == Engine::FontType_CFF)
  {
    engine_->setCFFHintingMode(
        hintingModeComboBoxModel_->indexToCFFMode(index));
    currentCFFHintingMode_ = index;
  }
  else if (engine_->currentFontType() == Engine::FontType_TrueType)
  {
    engine_->setTTInterpreterVersion(
        hintingModeComboBoxModel_->indexToTTInterpreterVersion(index));
    currentTTInterpreterVersion_ = index;
  }

  emit fontReloadNeeded();
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
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &SettingPanel::repaintNeeded);

  connect(gammaSlider_, &QSlider::valueChanged,
          this, &SettingPanel::repaintNeeded);
  
  connect(hintingCheckBox_, &QCheckBox::clicked,
          this, &SettingPanel::onFontChanged);

  connect(horizontalHintingCheckBox_, &QCheckBox::clicked,
          this, &SettingPanel::repaintNeeded);
  connect(verticalHintingCheckBox_, &QCheckBox::clicked,
          this, &SettingPanel::repaintNeeded);
  connect(blueZoneHintingCheckBox_, &QCheckBox::clicked,
          this, &SettingPanel::repaintNeeded);
  connect(segmentDrawingCheckBox_, &QCheckBox::clicked,
          this, &SettingPanel::repaintNeeded);

  connect(autoHintingCheckBox_, &QCheckBox::clicked,
          this, &SettingPanel::checkAutoHinting);
  connect(embeddedBitmapCheckBox_, &QCheckBox::clicked,
          this, &SettingPanel::fontReloadNeeded);
  connect(colorLayerCheckBox_, &QCheckBox::clicked,
          this, &SettingPanel::checkPalette);
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

  hintingModeLayout_ = new QHBoxLayout;
  hintingModeLayout_->addWidget(hintingModeLabel_);
  hintingModeLayout_->addWidget(hintingModeComboBox_);

  horizontalHintingLayout_ = new QHBoxLayout;
  horizontalHintingLayout_->addSpacing(20); // XXX px
  horizontalHintingLayout_->addWidget(horizontalHintingCheckBox_);

  verticalHintingLayout_ = new QHBoxLayout;
  verticalHintingLayout_->addSpacing(20); // XXX px
  verticalHintingLayout_->addWidget(verticalHintingCheckBox_);

  blueZoneHintingLayout_ = new QHBoxLayout;
  blueZoneHintingLayout_->addSpacing(20); // XXX px
  blueZoneHintingLayout_->addWidget(blueZoneHintingCheckBox_);

  segmentDrawingLayout_ = new QHBoxLayout;
  segmentDrawingLayout_->addSpacing(20); // XXX px
  segmentDrawingLayout_->addWidget(segmentDrawingCheckBox_);

  antiAliasingLayout_ = new QHBoxLayout;
  antiAliasingLayout_->addWidget(antiAliasingLabel_);
  antiAliasingLayout_->addWidget(antiAliasingComboBox_);

  lcdFilterLayout_ = new QHBoxLayout;
  lcdFilterLayout_->addWidget(lcdFilterLabel_);
  lcdFilterLayout_->addWidget(lcdFilterComboBox_);

  gammaLayout_ = new QHBoxLayout;
  gammaLayout_->addWidget(gammaLabel_);
  gammaLayout_->addWidget(gammaSlider_);

  paletteLayout_ = new QHBoxLayout;
  paletteLayout_->addWidget(paletteLabel_);
  paletteLayout_->addWidget(paletteComboBox_);

  generalTabLayout_ = new QVBoxLayout;
  generalTabLayout_->addWidget(hintingCheckBox_);
  generalTabLayout_->addLayout(hintingModeLayout_);
  generalTabLayout_->addWidget(autoHintingCheckBox_);
  generalTabLayout_->addLayout(horizontalHintingLayout_);
  generalTabLayout_->addLayout(verticalHintingLayout_);
  generalTabLayout_->addLayout(blueZoneHintingLayout_);
  generalTabLayout_->addLayout(segmentDrawingLayout_);
  generalTabLayout_->addSpacing(20); // XXX px
  generalTabLayout_->addStretch(1);
  generalTabLayout_->addLayout(antiAliasingLayout_);
  generalTabLayout_->addLayout(lcdFilterLayout_);
  generalTabLayout_->addSpacing(20); // XXX px
  generalTabLayout_->addStretch(1);
  generalTabLayout_->addLayout(gammaLayout_);
  generalTabLayout_->addWidget(embeddedBitmapCheckBox_);
  generalTabLayout_->addWidget(colorLayerCheckBox_);
  generalTabLayout_->addLayout(paletteLayout_);
  generalTabLayout_->addSpacing(20); // XXX px
  generalTabLayout_->addStretch(1);

  generalTab_ = new QWidget(this);
  generalTab_->setLayout(generalTabLayout_);

  mmgxTab_ = new QWidget(this);

  tab_ = new QTabWidget(this);
  tab_->addTab(generalTab_, tr("General"));
  tab_->addTab(mmgxTab_, tr("MM/GX"));

  mainLayout_ = new QVBoxLayout;
  mainLayout_->addWidget(tab_);
  setLayout(mainLayout_);
  mainLayout_->setContentsMargins(0, 0, 0, 0);
  setContentsMargins(0, 0, 0, 0);
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

  gammaSlider_->setValue(18); // 1.8
}


// end of settingpanel.cpp
