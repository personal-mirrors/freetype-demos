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

  std::vector<CharMapInfo> tempCharMaps;
  setCharMaps(tempCharMaps); // pass in an empty one

  checkModeSource();
  setDefaults();

  createConnections();
}


void
ContinuousTab::repaintGlyph()
{
  sizeSelector_->applyToEngine(engine_);
  
  syncSettings();
  canvas_->repaint();
}


void
ContinuousTab::reloadFont()
{
  currentGlyphCount_ = engine_->currentFontNumberOfGlyphs();
  setGlyphCount(qBound(0, currentGlyphCount_, INT_MAX));
  setCharMaps(engine_->currentFontCharMaps());
  canvas_->stringRenderer().reloadAll();
  repaintGlyph();
}


void
ContinuousTab::syncSettings()
{
  auto mode = static_cast<GlyphContinuous::Mode>(modeSelector_->currentIndex());
  auto src
    = static_cast<GlyphContinuous::Source>(sourceSelector_->currentIndex());
  canvas_->setMode(mode);
  canvas_->setSource(src);
  canvas_->setBeginIndex(indexSelector_->currentIndex());
  auto& sr = canvas_->stringRenderer();
  sr.setWaterfall(waterfallCheckBox_->isChecked());
  sr.setVertical(verticalCheckBox_->isChecked());
  sr.setKerning(kerningCheckBox_->isChecked());
  sr.setRotation(rotationSpinBox_->value());
  sr.setPosition(positionSlider_->value() / 100.0);

  // Not directly from the combo box
  sr.setCharMapIndex(charMapIndex(), glyphLimitIndex_);

  //sr.setCentered(centered_->isChecked());

  canvas_->setFancyParams(xEmboldeningSpinBox_->value(),
                          yEmboldeningSpinBox_->value(),
                          slantSpinBox_->value());
  canvas_->setStrokeRadius(strokeRadiusSpinBox_->value());
}


int
ContinuousTab::charMapIndex()
{
  auto index = charMapSelector_->currentIndex() - 1;
  if (index <= -1)
    return -1;
  if (static_cast<unsigned>(index) >= charMaps_.size())
    return -1;
  return index;
}


void
ContinuousTab::setGlyphCount(int count)
{
  currentGlyphCount_ = count;
  updateLimitIndex();
}


void
ContinuousTab::setDisplayingCount(int count)
{
  indexSelector_->setShowingCount(count);
}


void
ContinuousTab::setGlyphBeginindex(int index)
{
  indexSelector_->setCurrentIndex(index);
}


#define EncodingRole (Qt::UserRole + 10)
void
ContinuousTab::setCharMaps(std::vector<CharMapInfo>& charMaps)
{
  if (charMaps_ == charMaps)
  {
    charMaps_ = charMaps; // Still need to substitute because ptr may differ
    return;
  }
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
      charMapSelector_->addItem(tr("%1: %2 (platform %3, encoding %4)")
                                .arg(i)
                                .arg(*map.encodingName)
                                .arg(map.platformID)
                                .arg(map.encodingID));
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
ContinuousTab::updateLimitIndex()
{
  if (charMapSelector_->currentIndex() <= 0)
    glyphLimitIndex_ = currentGlyphCount_;
  else
    glyphLimitIndex_
      = charMaps_[charMapSelector_->currentIndex() - 1].maxIndex + 1;
  indexSelector_->setMinMax(0, glyphLimitIndex_ - 1);
}


void
ContinuousTab::checkModeSource()
{
  auto isFancy = modeSelector_->currentIndex() == GlyphContinuous::M_Fancy;
  auto isStroked = modeSelector_->currentIndex() == GlyphContinuous::M_Stroked;
  xEmboldeningSpinBox_->setEnabled(isFancy);
  yEmboldeningSpinBox_->setEnabled(isFancy);
  slantSpinBox_->setEnabled(isFancy);
  strokeRadiusSpinBox_->setEnabled(isStroked);

  auto src
      = static_cast<GlyphContinuous::Source>(sourceSelector_->currentIndex());
  auto isTextStrict = src == GlyphContinuous::SRC_TextString;
  auto isText = src == GlyphContinuous::SRC_TextString
                || src == GlyphContinuous::SRC_TextStringRepeated;
  indexSelector_->setEnabled(src == GlyphContinuous::SRC_AllGlyphs);
  sourceTextEdit_->setEnabled(isText);
  positionSlider_->setEnabled(isText);
  canvas_->setSource(src);

  {
    auto wf = waterfallCheckBox_->isChecked();
    QSignalBlocker blocker(verticalCheckBox_);
    if (wf || !isTextStrict)
      verticalCheckBox_->setChecked(false);
    verticalCheckBox_->setEnabled(!wf && isTextStrict);
  }

  {
    auto vert = verticalCheckBox_->isChecked();
    QSignalBlocker blocker(waterfallCheckBox_);
    if (vert)
      waterfallCheckBox_->setChecked(false);
    waterfallCheckBox_->setEnabled(!vert);
  }

  repaintGlyph();
}


void
ContinuousTab::charMapChanged()
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

  canvas_->stringRenderer().reloadAll();
  repaintGlyph();
  lastCharMapIndex_ = newIndex;
}


void
ContinuousTab::sourceTextChanged()
{
  canvas_->setSourceText(sourceTextEdit_->toPlainText());
  repaintGlyph();
}


void
ContinuousTab::reloadGlyphsAndRepaint()
{
  canvas_->stringRenderer().reloadGlyphs();
  repaintGlyph();
}


bool
ContinuousTab::eventFilter(QObject* watched,
                           QEvent* event)
{
  if (event->type() == QEvent::KeyPress)
  {
    auto keyEvent = dynamic_cast<QKeyEvent*>(event);
    if (sizeSelector_->handleKeyEvent(keyEvent))
      return true;
  }
  return false;
}


void
ContinuousTab::wheelNavigate(int steps)
{
  if (sourceSelector_->currentIndex() == GlyphContinuous::SRC_AllGlyphs)
    setGlyphBeginindex(indexSelector_->currentIndex() + steps);
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

  indexSelector_ = new GlyphIndexSelector(this);
  indexSelector_->setSingleMode(false);
  indexSelector_->setNumberRenderer([this](int index)
                                    { return formatIndex(index); });
  sourceTextEdit_ = new QPlainTextEdit(
      tr("The quick brown fox jumps over the lazy dog."), this);

  modeSelector_ = new QComboBox(this);
  charMapSelector_ = new QComboBox(this);
  sourceSelector_ = new QComboBox(this);

  charMapSelector_->setSizeAdjustPolicy(QComboBox::AdjustToContents);

  // Note: in sync with the enum!!
  modeSelector_->insertItem(GlyphContinuous::M_Normal, tr("Normal"));
  modeSelector_->insertItem(GlyphContinuous::M_Fancy, tr("Fancy"));
  modeSelector_->insertItem(GlyphContinuous::M_Stroked, tr("Stroked"));
  modeSelector_->setCurrentIndex(GlyphContinuous::M_Normal);

  // Note: in sync with the enum!!
  sourceSelector_->insertItem(GlyphContinuous::SRC_AllGlyphs,
                              tr("All Glyphs"));
  sourceSelector_->insertItem(GlyphContinuous::SRC_TextString, 
                              tr("Text String"));
  sourceSelector_->insertItem(GlyphContinuous::SRC_TextStringRepeated,
                              tr("Text String (Repeated)"));

  verticalCheckBox_ = new QCheckBox(tr("Vertical"), this);
  waterfallCheckBox_ = new QCheckBox(tr("Waterfall"), this);
  kerningCheckBox_ = new QCheckBox(tr("Kerning"), this);

  positionSlider_ = new QSlider(Qt::Horizontal, this);
  positionSlider_->setMinimum(0);
  positionSlider_->setMaximum(100);
  positionSlider_->setTracking(true);
  positionSlider_->setSingleStep(5);

  modeLabel_ = new QLabel(tr("Mode:"), this);
  sourceLabel_ = new QLabel(tr("Text Source:"), this);
  charMapLabel_ = new QLabel(tr("Char Map:"), this);
  xEmboldeningLabel_ = new QLabel(tr("Horz. Emb.:"), this);
  yEmboldeningLabel_ = new QLabel(tr("Vert. Emb.:"), this);
  slantLabel_ = new QLabel(tr("Slanting:"), this);
  strokeRadiusLabel_ = new QLabel(tr("Stroke Radius:"), this);
  rotationLabel_ = new QLabel(tr("Rotation:"), this);
  positionLabel_ = new QLabel(tr("Pos"), this);

  positionLabel_->setAlignment(Qt::AlignCenter);

  xEmboldeningSpinBox_ = new QDoubleSpinBox(this);
  yEmboldeningSpinBox_ = new QDoubleSpinBox(this);
  slantSpinBox_ = new QDoubleSpinBox(this);
  strokeRadiusSpinBox_ = new QDoubleSpinBox(this);
  rotationSpinBox_ = new QDoubleSpinBox(this);

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
  rotationSpinBox_->setSingleStep(5);
  rotationSpinBox_->setMinimum(-180);
  rotationSpinBox_->setMaximum(180);

  positionLayout_ = new QVBoxLayout;
  positionLayout_->addWidget(positionLabel_);
  positionLayout_->addWidget(positionSlider_);

  bottomLayout_ = new QGridLayout;
  bottomLayout_->addWidget(sourceLabel_, 0, 0);
  bottomLayout_->addWidget(modeLabel_, 1, 0);
  bottomLayout_->addWidget(charMapLabel_, 2, 0);
  bottomLayout_->addWidget(sourceSelector_, 0, 1);
  bottomLayout_->addWidget(modeSelector_, 1, 1);
  bottomLayout_->addWidget(charMapSelector_, 2, 1);

  bottomLayout_->addWidget(xEmboldeningLabel_, 1, 2);
  bottomLayout_->addWidget(yEmboldeningLabel_, 2, 2);
  bottomLayout_->addWidget(slantLabel_, 3, 2);
  bottomLayout_->addWidget(strokeRadiusLabel_, 3, 0);
  bottomLayout_->addWidget(rotationLabel_, 0, 2);

  bottomLayout_->addWidget(xEmboldeningSpinBox_, 1, 3);
  bottomLayout_->addWidget(yEmboldeningSpinBox_, 2, 3);
  bottomLayout_->addWidget(slantSpinBox_, 3, 3);
  bottomLayout_->addWidget(strokeRadiusSpinBox_, 3, 1);
  bottomLayout_->addWidget(rotationSpinBox_, 0, 3);

  bottomLayout_->addWidget(indexSelector_, 0, 4, 1, 1);
  bottomLayout_->addWidget(sourceTextEdit_, 1, 4, 3, 1);
  bottomLayout_->addLayout(positionLayout_, 0, 5);
  bottomLayout_->addWidget(waterfallCheckBox_, 1, 5);
  bottomLayout_->addWidget(verticalCheckBox_, 2, 5);
  bottomLayout_->addWidget(kerningCheckBox_, 3, 5);

  bottomLayout_->setColumnStretch(4, 1);

  mainLayout_ = new QVBoxLayout;
  mainLayout_->addWidget(canvas_);
  mainLayout_->addWidget(sizeSelector_);
  mainLayout_->addLayout(bottomLayout_);

  setLayout(mainLayout_);
}


void
ContinuousTab::createConnections()
{
  connect(sizeSelector_, &FontSizeSelector::valueChanged,
          this, &ContinuousTab::reloadGlyphsAndRepaint);

  connect(canvas_, &GlyphContinuous::wheelResize, 
          this, &ContinuousTab::wheelResize);
  connect(canvas_, &GlyphContinuous::wheelNavigate, 
          this, &ContinuousTab::wheelNavigate);
  connect(canvas_, &GlyphContinuous::displayingCountUpdated, 
          this, &ContinuousTab::setDisplayingCount);

  connect(indexSelector_, &GlyphIndexSelector::currentIndexChanged,
          this, &ContinuousTab::repaintGlyph);
  connect(modeSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ContinuousTab::checkModeSource);
  connect(charMapSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ContinuousTab::charMapChanged);
  connect(sourceSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ContinuousTab::checkModeSource);

  connect(xEmboldeningSpinBox_, 
          QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &ContinuousTab::repaintGlyph);
  connect(yEmboldeningSpinBox_, 
          QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &ContinuousTab::repaintGlyph);
  connect(slantSpinBox_, 
          QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &ContinuousTab::repaintGlyph);
  connect(strokeRadiusSpinBox_, 
          QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &ContinuousTab::repaintGlyph);
  connect(rotationSpinBox_, 
          QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &ContinuousTab::repaintGlyph);

  connect(waterfallCheckBox_, &QCheckBox::clicked,
          this, &ContinuousTab::checkModeSource);
  connect(verticalCheckBox_, &QCheckBox::clicked,
          this, &ContinuousTab::checkModeSource);
  connect(kerningCheckBox_, &QCheckBox::clicked,
          this, &ContinuousTab::reloadGlyphsAndRepaint);
  connect(sourceTextEdit_, &QPlainTextEdit::textChanged,
          this, &ContinuousTab::sourceTextChanged);

  connect(positionSlider_, &QSlider::valueChanged,
          this, &ContinuousTab::repaintGlyph);

  sizeSelector_->installEventFilterForWidget(canvas_);
  sizeSelector_->installEventFilterForWidget(this);
}


void
ContinuousTab::setDefaults()
{
  xEmboldeningSpinBox_->setValue(0.04);
  yEmboldeningSpinBox_->setValue(0.04);
  slantSpinBox_->setValue(0.22);
  strokeRadiusSpinBox_->setValue(0.02);
  rotationSpinBox_->setValue(0);

  positionSlider_->setValue(50);

  canvas_->setSourceText(sourceTextEdit_->toPlainText());
  canvas_->setSource(GlyphContinuous::SRC_AllGlyphs);
}


QString
ContinuousTab::formatIndex(int index)
{
  if (charMapSelector_->currentIndex() <= 0) // glyph order
    return QString::number(index);
  return charMaps_[charMapSelector_->currentIndex() - 1].stringifyIndexShort(
      index);
}


// end of continuous.cpp
