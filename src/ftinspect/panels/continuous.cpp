// continuous.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "continuous.hpp"

#include <climits>
#include <QVariant>


ContinuousTab::ContinuousTab(QWidget* parent,
                             Engine* engine)
: QWidget(parent), engine_(engine)
{
  createLayout();
  createConnections();
}


void
ContinuousTab::repaintGlyph()
{
  sizeSelector_->applyToEngine(engine_);
  
  updateFromCurrentSubTab();
  canvas_->repaint();
}


void
ContinuousTab::reloadFont()
{
  currentGlyphCount_ = engine_->currentFontNumberOfGlyphs();
  updateCurrentSubTab();
  repaintGlyph();
}


void
ContinuousTab::changeTab()
{
  updateCurrentSubTab();
  repaintGlyph();
}


void
ContinuousTab::wheelNavigate(int steps)
{
  if (tabWidget_->currentIndex() == AllGlyphs)
    allGlyphsTab_->setGlyphBeginindex(allGlyphsTab_->glyphBeginindex()
                                      + steps);
}


void
ContinuousTab::wheelResize(int steps)
{
  sizeSelector_->handleWheelResizeBySteps(steps);
}


void
ContinuousTab::createLayout()
{
  canvas_ = new GlyphContinuous(this, engine_);
  sizeSelector_ = new FontSizeSelector(this);
  allGlyphsTab_ = new ContinousAllGlyphsTab(this);

  tabWidget_ = new QTabWidget(this);
  tabWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  // Must be in sync with `Tabs` enum.
  tabWidget_->addTab(allGlyphsTab_, tr("All Glyphs"));

  mainLayout_ = new QVBoxLayout;
  mainLayout_->addWidget(canvas_);
  mainLayout_->addWidget(sizeSelector_);
  mainLayout_->addWidget(tabWidget_);

  setLayout(mainLayout_);
}


void
ContinuousTab::createConnections()
{
  connect(tabWidget_, &QTabWidget::currentChanged,
          this, &ContinuousTab::changeTab);

  connect(allGlyphsTab_, &ContinousAllGlyphsTab::changed, 
          this, &ContinuousTab::repaintGlyph);

  connect(sizeSelector_, &FontSizeSelector::valueChanged,
          this, &ContinuousTab::repaintGlyph);

  connect(canvas_, &GlyphContinuous::wheelResize, 
          this, &ContinuousTab::wheelResize);
  connect(canvas_, &GlyphContinuous::wheelNavigate, 
          this, &ContinuousTab::wheelNavigate);
  connect(canvas_, &GlyphContinuous::displayingCountUpdated, 
          allGlyphsTab_, &ContinousAllGlyphsTab::setDisplayingCount);
}


void
ContinuousTab::updateCurrentSubTab()
{
  switch (tabWidget_->currentIndex())
  {
  case AllGlyphs:
    allGlyphsTab_->setGlyphCount(qBound(0, 
                                        currentGlyphCount_,
                                        INT_MAX));
    allGlyphsTab_->setCharMaps(engine_->currentFontCharMaps());
    break;
  }
}


void
ContinuousTab::updateFromCurrentSubTab()
{
  switch (tabWidget_->currentIndex())
  {
  case AllGlyphs:
    canvas_->setMode(GlyphContinuous::AllGlyphs);
    canvas_->setSubModeAllGlyphs(allGlyphsTab_->subMode());
    // Begin index is selected from All Glyphs subtab,
    // and Limit index is calculated by All Glyphs subtab
    canvas_->setBeginIndex(allGlyphsTab_->glyphBeginindex());
    canvas_->setLimitIndex(allGlyphsTab_->glyphLimitIndex());
    canvas_->setCharMapIndex(allGlyphsTab_->charMapIndex());

    canvas_->setFancyParams(allGlyphsTab_->xEmboldening(),
                            allGlyphsTab_->yEmboldening(),
                            allGlyphsTab_->slanting());
    canvas_->setStrokeRadius(allGlyphsTab_->strokeRadius());
    break;
  }
}


ContinousAllGlyphsTab::ContinousAllGlyphsTab(QWidget* parent)
: QWidget(parent)
{
  createLayout();

  std::vector<CharMapInfo> tempCharMaps;
  setCharMaps(tempCharMaps); // pass in an empty one

  checkSubMode();
  setDefaults();
  createConnections();
}


int
ContinousAllGlyphsTab::glyphBeginindex()
{
  return indexSelector_->currentIndex();
}


int
ContinousAllGlyphsTab::glyphLimitIndex()
{
  return glyphLimitIndex_;
}


GlyphContinuous::SubModeAllGlyphs
ContinousAllGlyphsTab::subMode()
{
  return static_cast<GlyphContinuous::SubModeAllGlyphs>(
           modeSelector_->currentIndex());
}


double
ContinousAllGlyphsTab::xEmboldening()
{
  return xEmboldeningSpinBox_->value();
}


double
ContinousAllGlyphsTab::yEmboldening()
{
  return yEmboldeningSpinBox_->value();
}


double
ContinousAllGlyphsTab::slanting()
{
  return slantSpinBox_->value();
}


double
ContinousAllGlyphsTab::strokeRadius()
{
  return strokeRadiusSpinBox_->value();
}


int
ContinousAllGlyphsTab::charMapIndex()
{
  auto index = charMapSelector_->currentIndex() - 1;
  if (index <= -1)
    return -1;
  if (static_cast<unsigned>(index) >= charMaps_.size())
    return -1;
  return index;
}


void
ContinousAllGlyphsTab::setGlyphBeginindex(int index)
{
  indexSelector_->setCurrentIndex(index);
}


void
ContinousAllGlyphsTab::setGlyphCount(int count)
{
  currentGlyphCount_ = count;
  updateLimitIndex();
}


void
ContinousAllGlyphsTab::setDisplayingCount(int count)
{
  indexSelector_->setShowingCount(count);
}


#define EncodingRole (Qt::UserRole + 10)
void
ContinousAllGlyphsTab::setCharMaps(std::vector<CharMapInfo>& charMaps)
{
  charMaps_ = charMaps;
  int oldIndex = charMapSelector_->currentIndex();
  unsigned oldEncoding = 0u;

  // Using additional UserRole to store encoding id
  auto oldEncodingV = charMapSelector_->itemData(oldIndex, EncodingRole);
  if (oldEncodingV.isValid() && oldEncodingV.canConvert<unsigned>())
  {
    oldEncoding = oldEncodingV.value<unsigned>();
  }

  {
    // suppress events during updating
    QSignalBlocker selectorBlocker(charMapSelector_);

    charMapSelector_->clear();
    charMapSelector_->addItem(tr("Glyph Order"));
    charMapSelector_->setItemData(0, 0u, EncodingRole);

    int i = 0;
    int newIndex = 0;
    for (auto& map : charMaps)
    {
      charMapSelector_->addItem(tr("%1: %2")
                                .arg(i)
                                .arg(*map.encodingName));
      auto encoding = static_cast<unsigned>(map.encoding);
      charMapSelector_->setItemData(i, encoding, EncodingRole);

      if (encoding == oldEncoding && i == oldIndex)
        newIndex = i;
    
      i++;
    }

    // this shouldn't emit any event either, because force repainting
    // will happen later, so embrace it into blocker block
    charMapSelector_->setCurrentIndex(newIndex);
  }

  updateLimitIndex();
}


void
ContinousAllGlyphsTab::updateLimitIndex()
{
  if (charMapSelector_->currentIndex() <= 0)
    glyphLimitIndex_ = currentGlyphCount_;
  else
    glyphLimitIndex_
      = charMaps_[charMapSelector_->currentIndex() - 1].maxIndex + 1;
  indexSelector_->setMinMax(0, glyphLimitIndex_ - 1);
}


void
ContinousAllGlyphsTab::checkSubMode()
{
  auto isFancy = subMode() == GlyphContinuous::AG_Fancy;
  auto isStroked = subMode() == GlyphContinuous::AG_Stroked;
  xEmboldeningSpinBox_->setEnabled(isFancy);
  yEmboldeningSpinBox_->setEnabled(isFancy);
  slantSpinBox_->setEnabled(isFancy);
  strokeRadiusSpinBox_->setEnabled(isStroked);

  emit changed();
}


void
ContinousAllGlyphsTab::createLayout()
{
  indexSelector_ = new GlyphIndexSelector(this);
  indexSelector_->setSingleMode(false);
  indexSelector_->setNumberRenderer([this](int index)
                                    { return formatIndex(index); });

  modeSelector_ = new QComboBox(this);
  charMapSelector_ = new QComboBox(this);

  // Note: in sync with the enum!!
  modeSelector_->insertItem(GlyphContinuous::AG_AllGlyphs, tr("All Glyphs"));
  modeSelector_->insertItem(GlyphContinuous::AG_Fancy, 
                            tr("Fancy (Embolding & Slanting)"));
  modeSelector_->insertItem(GlyphContinuous::AG_Stroked, tr("Stroked"));
  modeSelector_->insertItem(GlyphContinuous::AG_Waterfall, tr("Waterfall"));
  modeSelector_->setCurrentIndex(GlyphContinuous::AG_AllGlyphs);

  modeLabel_ = new QLabel(tr("Mode:"), this);
  charMapLabel_ = new QLabel(tr("Char Map:"), this);
  xEmboldeningLabel_ = new QLabel(tr("Hori. Embolding:"), this);
  yEmboldeningLabel_ = new QLabel(tr("Vert. Embolding:"), this);
  slantLabel_ = new QLabel(tr("Slanting:"), this);
  strokeRadiusLabel_ = new QLabel(tr("Stroke Radius:"), this);

  xEmboldeningSpinBox_ = new QDoubleSpinBox(this);
  yEmboldeningSpinBox_ = new QDoubleSpinBox(this);
  slantSpinBox_ = new QDoubleSpinBox(this);
  strokeRadiusSpinBox_ = new QDoubleSpinBox(this);

  xEmboldeningSpinBox_->setSingleStep(0.005);
  xEmboldeningSpinBox_->setMinimum(-0.1);
  xEmboldeningSpinBox_->setMaximum(0.1);
  yEmboldeningSpinBox_->setSingleStep(0.005);
  yEmboldeningSpinBox_->setMinimum(-0.1);
  yEmboldeningSpinBox_->setMaximum(0.1);
  slantSpinBox_->setSingleStep(0.02);
  slantSpinBox_->setMinimum(-1);
  slantSpinBox_->setMaximum(1);
  strokeRadiusSpinBox_->setSingleStep(0.005);
  strokeRadiusSpinBox_->setMinimum(0);
  strokeRadiusSpinBox_->setMaximum(0.05);

  layout_ = new QGridLayout;
  layout_->addWidget(indexSelector_, 0, 0, 1, 2);
  layout_->addWidget(modeLabel_, 1, 0);
  layout_->addWidget(charMapLabel_, 2, 0);
  layout_->addWidget(modeSelector_, 1, 1);
  layout_->addWidget(charMapSelector_, 2, 1);

  layout_->addWidget(xEmboldeningLabel_, 1, 2);
  layout_->addWidget(yEmboldeningLabel_, 2, 2);
  layout_->addWidget(slantLabel_, 3, 2);
  layout_->addWidget(strokeRadiusLabel_, 3, 0);
  layout_->addWidget(xEmboldeningSpinBox_, 1, 3);
  layout_->addWidget(yEmboldeningSpinBox_, 2, 3);
  layout_->addWidget(slantSpinBox_, 3, 3);
  layout_->addWidget(strokeRadiusSpinBox_, 3, 1);

  layout_->setColumnStretch(1, 1);
  layout_->setColumnStretch(3, 1);

  setLayout(layout_);
}

void
ContinousAllGlyphsTab::createConnections()
{
  connect(indexSelector_, &GlyphIndexSelector::currentIndexChanged,
          this, &ContinousAllGlyphsTab::changed);
  connect(modeSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ContinousAllGlyphsTab::checkSubMode);
  connect(charMapSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ContinousAllGlyphsTab::charMapChanged);

  connect(xEmboldeningSpinBox_, 
          QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &ContinousAllGlyphsTab::changed);
  connect(yEmboldeningSpinBox_, 
          QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &ContinousAllGlyphsTab::changed);
  connect(slantSpinBox_, 
          QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &ContinousAllGlyphsTab::changed);
  connect(strokeRadiusSpinBox_, 
          QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &ContinousAllGlyphsTab::changed);
}


QString
ContinousAllGlyphsTab::formatIndex(int index)
{
  if (charMapSelector_->currentIndex() <= 0) // glyph order
    return QString::number(index);
  return charMaps_[charMapSelector_->currentIndex() - 1]
           .stringifyIndexShort(index);
}


void
ContinousAllGlyphsTab::charMapChanged()
{
  int newIndex = charMapSelector_->currentIndex();
  if (newIndex != lastCharMapIndex_)
  {
    if (newIndex <= 0
        || charMaps_.size() <= static_cast<unsigned>(newIndex - 1))
      setGlyphBeginindex(0);
    else if (charMaps_[newIndex - 1].maxIndex <= 20)
      setGlyphBeginindex(charMaps_[newIndex - 1].maxIndex - 1);
    else
      setGlyphBeginindex(0x20);
  }
  updateLimitIndex();

  emit changed();

  lastCharMapIndex_ = newIndex;
}


void
ContinousAllGlyphsTab::setDefaults()
{
  xEmboldeningSpinBox_->setValue(0.04);
  yEmboldeningSpinBox_->setValue(0.04);
  slantSpinBox_->setValue(0.22);
  strokeRadiusSpinBox_->setValue(0.02);
}


// end of continuous.cpp
