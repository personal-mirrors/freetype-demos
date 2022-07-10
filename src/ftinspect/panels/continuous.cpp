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
    // Begin index is selected from All Glyphs subtab,
    // and Limit index is calculated by All Glyphs subtab
    canvas_->setBeginIndex(allGlyphsTab_->glyphBeginindex());
    canvas_->setLimitIndex(allGlyphsTab_->glyphLimitIndex());
    canvas_->setMode(GlyphContinuous::AllGlyphs);
    canvas_->setSubModeAllGlyphs(allGlyphsTab_->subMode());
    canvas_->setCharMapIndex(allGlyphsTab_->charMapIndex());
    break;
  }
}


ContinousAllGlyphsTab::ContinousAllGlyphsTab(QWidget* parent)
: QWidget(parent)
{
  createLayout();

  QVector<CharMapInfo> tempCharMaps;
  setCharMaps(tempCharMaps); // pass in an empty one

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


int
ContinousAllGlyphsTab::charMapIndex()
{
  auto index = charMapSelector_->currentIndex() - 1;
  if (index <= -1)
    return -1;
  if (index >= charMaps_.size())
    return -1;
  return index;
}


void
ContinousAllGlyphsTab::setGlyphBeginindex(int index)
{
  indexSelector_->setCurrentIndex(index);
  updateCharMapLimit();
}


void
ContinousAllGlyphsTab::setDisplayingCount(int count)
{
  indexSelector_->setShowingCount(count);
}


#define EncodingRole (Qt::UserRole + 10)
void
ContinousAllGlyphsTab::setCharMaps(QVector<CharMapInfo>& charMaps)
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

  updateCharMapLimit();
}


void
ContinousAllGlyphsTab::updateCharMapLimit()
{
  if (charMapSelector_->currentIndex() <= 0)
    glyphLimitIndex_ = currentGlyphCount_;
  else
    glyphLimitIndex_
      = charMaps_[charMapSelector_->currentIndex() - 1].maxIndex + 1;
  indexSelector_->setMinMax(0, glyphLimitIndex_ - 1);
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
  modeSelector_->insertItem(GlyphContinuous::AG_Fancy, tr("Fancy"));
  modeSelector_->insertItem(GlyphContinuous::AG_Stroked, tr("Stroked"));
  modeSelector_->insertItem(GlyphContinuous::AG_Waterfall, tr("Waterfall"));
  modeSelector_->setCurrentIndex(GlyphContinuous::AG_AllGlyphs);

  modeLabel_ = new QLabel(tr("Mode:"), this);
  charMapLabel_ = new QLabel(tr("Char Map:"), this);

  layout_ = new QGridLayout;
  layout_->addWidget(indexSelector_, 0, 0, 1, 2);
  layout_->addWidget(modeLabel_, 1, 0);
  layout_->addWidget(charMapLabel_, 2, 0);
  layout_->addWidget(modeSelector_, 1, 1);
  layout_->addWidget(charMapSelector_, 2, 1);

  layout_->setColumnStretch(1, 1);

  setLayout(layout_);
}

void
ContinousAllGlyphsTab::createConnections()
{
  connect(indexSelector_, &GlyphIndexSelector::currentIndexChanged,
          this, &ContinousAllGlyphsTab::changed);
  connect(modeSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ContinousAllGlyphsTab::changed);
  connect(charMapSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ContinousAllGlyphsTab::charMapChanged);
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
    if (newIndex <= 0 || charMaps_.size() <= newIndex - 1)
      setGlyphBeginindex(0);
    else if (charMaps_[newIndex - 1].maxIndex <= 20)
      setGlyphBeginindex(charMaps_[newIndex - 1].maxIndex - 1);
    else
      setGlyphBeginindex(0x20);
  }
  updateCharMapLimit();

  emit changed();

  lastCharMapIndex_ = newIndex;
}


// end of continuous.cpp
