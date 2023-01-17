// continuous.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "continuous.hpp"
#include "glyphdetails.hpp"
#include "../uihelper.hpp"

#include <climits>

#include <QToolTip>
#include <QVariant>


ContinuousTab::ContinuousTab(QWidget* parent,
                             Engine* engine,
                             QDockWidget* gdWidget,
                             GlyphDetails* glyphDetails)
: QWidget(parent),
  engine_(engine),
  glyphDetailsWidget_(gdWidget),
  glyphDetails_(glyphDetails)
{
  createLayout();

  std::vector<CharMapInfo> tempCharMaps;
  charMapSelector_->repopulate(tempCharMaps); // Pass in an empty one.

  checkModeSource();
  setDefaults();

  createConnections();
}


void
ContinuousTab::repaintGlyph()
{
  sizeSelector_->applyToEngine(engine_);

  applySettings();
  canvas_->stopFlashing();
  canvas_->purgeCache();
  canvas_->repaint();
}


void
ContinuousTab::reloadFont()
{
  currentGlyphCount_ = engine_->currentFontNumberOfGlyphs();
  {
    QSignalBlocker blocker(sizeSelector_);
    sizeSelector_->reloadFromFont(engine_);
  }
  setGlyphCount(qBound(0, currentGlyphCount_, INT_MAX));
  checkModeSource();

  charMapSelector_->repopulate();
  canvas_->stopFlashing();
  canvas_->stringRenderer().reloadAll();
  canvas_->purgeCache();
  repaintGlyph();
}


void
ContinuousTab::applySettings()
{
  auto mode
    = static_cast<GlyphContinuous::Mode>(modeSelector_->currentIndex());
  auto src
    = static_cast<GlyphContinuous::Source>(sourceSelector_->currentIndex());
  canvas_->setMode(mode);
  canvas_->setSource(src);
  canvas_->setBeginIndex(indexSelector_->currentIndex());
  canvas_->setScale(sizeSelector_->zoomFactor());
  auto& sr = canvas_->stringRenderer();
  sr.setWaterfall(waterfallCheckBox_->isChecked());
  sr.setVertical(verticalCheckBox_->isChecked());
  sr.setKerning(kerningCheckBox_->isChecked());
  sr.setRotation(rotationSpinBox_->value());

  // -1: Glyph order, otherwise the char map index in the original list.
  sr.setCharMapIndex(charMapSelector_->currentCharMapIndex(),
                     glyphLimitIndex_);

  if (sr.isWaterfall())
    sr.setWaterfallParameters(wfConfigDialog_->startSize(),
                              wfConfigDialog_->endSize());

  canvas_->setFancyParams(xEmboldeningSpinBox_->value(),
                          yEmboldeningSpinBox_->value(),
                          slantSpinBox_->value());
  canvas_->setStrokeRadius(strokeRadiusSpinBox_->value());
}


void
ContinuousTab::highlightGlyph(int index)
{
  canvas_->flashOnGlyph(index);
}


void
ContinuousTab::setGlyphCount(int count)
{
  currentGlyphCount_ = count;
  updateLimitIndex();
}


void
ContinuousTab::setGlyphBeginindex(int index)
{
  indexSelector_->setCurrentIndex(index);
}


void
ContinuousTab::updateLimitIndex()
{
  auto cMap = charMapSelector_->currentCharMapIndex();
  if (cMap < 0)
    glyphLimitIndex_ = currentGlyphCount_;
  else
    glyphLimitIndex_ = charMapSelector_->charMaps()[cMap].maxIndex + 1;
  indexSelector_->setMinMax(0, glyphLimitIndex_ - 1);
}


void
ContinuousTab::checkModeSource()
{
  auto isFancy
    = modeSelector_->currentIndex() == GlyphContinuous::M_Fancy;
  auto isStroked
    = modeSelector_->currentIndex() == GlyphContinuous::M_Stroked;
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
  sampleStringSelector_->setEnabled(isText);

  {
    QSignalBlocker blocker(kerningCheckBox_);
    kerningCheckBox_->setEnabled(isText);
    if (!isText)
      kerningCheckBox_->setChecked(false);
  }

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

  waterfallConfigButton_->setEnabled(waterfallCheckBox_->isChecked()
                                     && !engine_->currentFontBitmapOnly());
}


void
ContinuousTab::checkModeSourceAndRepaint()
{
  checkModeSource();
  repaintGlyph();
}


void
ContinuousTab::charMapChanged()
{
  int newIndex = charMapSelector_->currentCharMapIndex();
  if (newIndex != lastCharMapIndex_)
    setGlyphBeginindex(charMapSelector_->defaultFirstGlyphIndex());
  updateLimitIndex();

  applySettings();
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
ContinuousTab::presetStringSelected()
{
  auto index = sampleStringSelector_->currentIndex();
  if (index < 0)
    return;

  auto var = sampleStringSelector_->currentData();
  if (var.isValid() && var.canConvert<QString>())
  {
    auto str = var.toString();
    if (!str.isEmpty())
      sourceTextEdit_->setPlainText(str);
  }
}


void
ContinuousTab::reloadGlyphsAndRepaint()
{
  canvas_->stringRenderer().reloadGlyphs();
  repaintGlyph();
}


void
ContinuousTab::updateGlyphDetails(GlyphCacheEntry* ctxt,
                                  int charMapIndex,
                                  bool open)
{
  glyphDetails_->updateGlyph(*ctxt, charMapIndex);
  if (open)
  {
    glyphDetailsWidget_->show();
    glyphDetailsWidget_->activateWindow();
  }
}


void
ContinuousTab::openWaterfallConfig()
{
  wfConfigDialog_->setVisible(true); // No `exec`: not modal.
}


void
ContinuousTab::showToolTip()
{
  QToolTip::showText(mapToGlobal(helpButton_->pos()),
                     tr(
R"(Shift + Scroll: Adjust Font Size
Ctrl + Scroll: Adjust Zoom Factor
Shift + Plus/Minus: Adjust Font Size
Shift + 0: Reset Font Size to Default
Left Click: Show Glyph Details Info
Right Click: Inspect Glyph in Singular Grid View

<All Glyphs Source>
  Drag: Adjust Begin Index
<Text String Source>
  Drag: Move String Position)"),
                     helpButton_);
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
ContinuousTab::wheelZoom(int steps)
{
  sizeSelector_->handleWheelZoomBySteps(steps);
}


void
ContinuousTab::wheelResize(int steps)
{
  sizeSelector_->handleWheelResizeBySteps(steps);
}


void
ContinuousTab::createLayout()
{
  canvasFrame_ = new QFrame(this);
  canvasFrame_->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);

  canvas_ = new GlyphContinuous(canvasFrame_, engine_);
  sizeSelector_ = new FontSizeSelector(this, false, true);

  indexSelector_ = new GlyphIndexSelector(this);
  indexSelector_->setSingleMode(false);
  indexSelector_->setNumberRenderer([this](int index)
                                    { return formatIndex(index); });
  sourceTextEdit_ = new QPlainTextEdit(
    tr("The quick brown fox jumps over the lazy dog."), this);

  modeSelector_ = new QComboBox(this);
  charMapSelector_ = new CharMapComboBox(this, engine_);
  sourceSelector_ = new QComboBox(this);
  sampleStringSelector_ = new QComboBox(this);

  charMapSelector_->setSizeAdjustPolicy(QComboBox::AdjustToContents);

  // Note: must be in sync with the enum!
  modeSelector_->insertItem(GlyphContinuous::M_Normal, tr("Normal"));
  modeSelector_->insertItem(GlyphContinuous::M_Fancy, tr("Fancy"));
  modeSelector_->insertItem(GlyphContinuous::M_Stroked, tr("Stroked"));
  modeSelector_->setCurrentIndex(GlyphContinuous::M_Normal);

  // Note: must be in sync with the enum!
  sourceSelector_->insertItem(GlyphContinuous::SRC_AllGlyphs,
                              tr("All Glyphs"));
  sourceSelector_->insertItem(GlyphContinuous::SRC_TextString,
                              tr("Text String"));
  sourceSelector_->insertItem(GlyphContinuous::SRC_TextStringRepeated,
                              tr("Text String (Repeated)"));

  verticalCheckBox_ = new QCheckBox(tr("Vertical"), this);
  waterfallCheckBox_ = new QCheckBox(tr("Waterfall"), this);
  kerningCheckBox_ = new QCheckBox(tr("Kerning"), this);

  modeLabel_ = new QLabel(tr("Mode:"), this);
  sourceLabel_ = new QLabel(tr("Text Source:"), this);
  charMapLabel_ = new QLabel(tr("Char Map:"), this);
  xEmboldeningLabel_ = new QLabel(tr("Horz. Emb.:"), this);
  yEmboldeningLabel_ = new QLabel(tr("Vert. Emb.:"), this);
  slantLabel_ = new QLabel(tr("Slanting:"), this);
  strokeRadiusLabel_ = new QLabel(tr("Stroke Radius:"), this);
  rotationLabel_ = new QLabel(tr("Rotation:"), this);

  resetPositionButton_ = new QPushButton(tr("Reset Pos"), this);
  waterfallConfigButton_ = new QPushButton(tr("WF Config"), this);
  helpButton_ = new QPushButton(this);
  helpButton_->setText(tr("?"));

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

  wfConfigDialog_ = new WaterfallConfigDialog(this);

  // Tooltips
  sourceSelector_->setToolTip(tr(
    "Choose what to display as the text source."));
  modeSelector_->setToolTip(tr(
    "Choose the special effect in which the text is displayed."));
  strokeRadiusSpinBox_->setToolTip(tr(
    "Stroke corner radius (only available when mode set to Stroked)"));
  rotationSpinBox_->setToolTip(tr("Rotation, in degrees"));
  xEmboldeningSpinBox_->setToolTip(tr(
    "Horizontal Emboldening (only available when mode set to Fancy)"));
  yEmboldeningSpinBox_->setToolTip(tr(
    "Vertical Emboldening (only available when mode set to Fancy)"));
  slantSpinBox_->setToolTip(tr(
    "Slanting (only available when mode set to Fancy)"));
  sourceTextEdit_->setToolTip(tr(
    "Source string (only available when source set to Text String)"));
  waterfallConfigButton_->setToolTip(tr(
    "Set waterfall start and end size. Not available when the font is not\n"
    "scalable because in such case all available sizes would be displayed."));
  sampleStringSelector_->setToolTip(tr(
    "Select preset sample strings (only available when source set to\n"
    "Text String)"));
  resetPositionButton_->setToolTip(tr(
    "Reset the position to the center (only available when source set to\n"
    "Text String)"));
  waterfallCheckBox_->setToolTip(tr(
    "Enable waterfall mode: show the font output in different sizes.\n"
    "Will show all available sizes when the font is not scalable."));
  verticalCheckBox_->setToolTip(tr(
    "Enable vertical rendering (only available\n"
    "when source set to Text String)"));
  kerningCheckBox_->setToolTip(tr(
    "Enable kerning (GPOS table unsupported)"));
  helpButton_->setToolTip(tr("Get mouse helps"));

  // Layouting
  canvasFrameLayout_ = new QHBoxLayout;
  canvasFrameLayout_->addWidget(canvas_);
  canvasFrame_->setLayout(canvasFrameLayout_);
  canvasFrameLayout_->setContentsMargins(2, 2, 2, 2);
  canvasFrame_->setContentsMargins(2, 2, 2, 2);

  sizeHelpLayout_ = new QHBoxLayout;
  sizeHelpLayout_->addWidget(sizeSelector_, 1, Qt::AlignVCenter);
  sizeHelpLayout_->addWidget(helpButton_, 0);

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

  bottomLayout_->addWidget(indexSelector_, 0, 4, 1, 2);
  bottomLayout_->addWidget(sourceTextEdit_, 1, 4, 3, 1);
  bottomLayout_->addWidget(resetPositionButton_, 0, 6);
  bottomLayout_->addWidget(waterfallCheckBox_, 1, 6);
  bottomLayout_->addWidget(verticalCheckBox_, 2, 6);
  bottomLayout_->addWidget(kerningCheckBox_, 3, 6);
  bottomLayout_->addWidget(waterfallConfigButton_, 1, 5);
  bottomLayout_->addWidget(sampleStringSelector_, 2, 5);

  bottomLayout_->setColumnStretch(4, 1);

  mainLayout_ = new QVBoxLayout;
  mainLayout_->addWidget(canvasFrame_);
  mainLayout_->addLayout(sizeHelpLayout_);
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
  connect(canvas_, &GlyphContinuous::wheelZoom,
          this, &ContinuousTab::wheelZoom);
  connect(canvas_, &GlyphContinuous::displayingCountUpdated,
          indexSelector_, &GlyphIndexSelector::setShowingCount);
  connect(canvas_, &GlyphContinuous::rightClickGlyph,
          this, &ContinuousTab::switchToSingular);
  connect(canvas_, &GlyphContinuous::beginIndexChangeRequest,
          this, &ContinuousTab::setGlyphBeginindex);
  connect(canvas_, &GlyphContinuous::updateGlyphDetails,
          this, &ContinuousTab::updateGlyphDetails);

  connect(indexSelector_, &GlyphIndexSelector::currentIndexChanged,
          this, &ContinuousTab::repaintGlyph);
  connect(modeSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ContinuousTab::checkModeSourceAndRepaint);
  connect(charMapSelector_,
          QOverload<int>::of(&CharMapComboBox::currentIndexChanged),
          this, &ContinuousTab::charMapChanged);
  connect(charMapSelector_, &CharMapComboBox::forceUpdateLimitIndex,
          this, &ContinuousTab::updateLimitIndex);
  connect(sourceSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ContinuousTab::checkModeSourceAndRepaint);

  connect(resetPositionButton_, &QPushButton::clicked,
          canvas_, &GlyphContinuous::resetPositionDelta);
  connect(waterfallConfigButton_, &QPushButton::clicked,
          this, &ContinuousTab::openWaterfallConfig);
  connect(helpButton_, &QPushButton::clicked,
          this, &ContinuousTab::showToolTip);
  connect(wfConfigDialog_, &WaterfallConfigDialog::sizeUpdated,
          this, &ContinuousTab::repaintGlyph);

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
          this, &ContinuousTab::checkModeSourceAndRepaint);
  connect(verticalCheckBox_, &QCheckBox::clicked,
          this, &ContinuousTab::checkModeSourceAndRepaint);
  connect(kerningCheckBox_, &QCheckBox::clicked,
          this, &ContinuousTab::reloadGlyphsAndRepaint);
  connect(sourceTextEdit_, &QPlainTextEdit::textChanged,
          this, &ContinuousTab::sourceTextChanged);
  connect(sampleStringSelector_,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ContinuousTab::presetStringSelected);

  sizeSelector_->installEventFilterForWidget(canvas_);
  sizeSelector_->installEventFilterForWidget(this);
}


extern const char* StringSamples[];

void
ContinuousTab::setDefaults()
{
  xEmboldeningSpinBox_->setValue(0.04);
  yEmboldeningSpinBox_->setValue(0.04);
  slantSpinBox_->setValue(0.22);
  strokeRadiusSpinBox_->setValue(0.02);
  rotationSpinBox_->setValue(0);

  canvas_->setSourceText(sourceTextEdit_->toPlainText());
  canvas_->setSource(GlyphContinuous::SRC_AllGlyphs);

  sampleStringSelector_->addItem(tr("<Sample>"));
  sampleStringSelector_->addItem(tr("English"),
                                 QString(StringSamples[0]));
  sampleStringSelector_->addItem(tr("Latin"),
                                 QString(StringSamples[1]));
  sampleStringSelector_->addItem(tr("Greek"),
                                 QString(StringSamples[2]));
  sampleStringSelector_->addItem(tr("Cyrillic"),
                                 QString(StringSamples[3]));
  sampleStringSelector_->addItem(tr("Chinese"),
                                 QString(StringSamples[4]));
  sampleStringSelector_->addItem(tr("Japanese"),
                                 QString(StringSamples[5]));
  sampleStringSelector_->addItem(tr("Korean"),
                                 QString(StringSamples[6]));
}


QString
ContinuousTab::formatIndex(int index)
{
  auto idx = charMapSelector_->currentCharMapIndex();
  if (idx < 0) // Glyph order.
    return QString::number(index);
  return charMapSelector_->charMaps()[idx].stringifyIndexShort(index);
}


WaterfallConfigDialog::WaterfallConfigDialog(QWidget* parent)
: QDialog(parent)
{
  setModal(false);
  setWindowTitle(tr("Waterfall Config"));
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

  createLayout();
  checkAutoStatus();
  createConnections();
}


double
WaterfallConfigDialog::startSize()
{
  if (autoBox_->isChecked())
    return -1.0;
  return startSpinBox_->value();
}


double
WaterfallConfigDialog::endSize()
{
  if (autoBox_->isChecked())
    return -1.0;
  return endSpinBox_->value();
}


void
WaterfallConfigDialog::createLayout()
{
  startLabel_ = new QLabel(tr("Start Size (pt):"), this);
  endLabel_ = new QLabel(tr("End Size (pt):"), this);

  startSpinBox_ = new QDoubleSpinBox(this);
  endSpinBox_ = new QDoubleSpinBox(this);

  startSpinBox_->setSingleStep(0.5);
  startSpinBox_->setMinimum(0.5);
  startSpinBox_->setValue(1);

  endSpinBox_->setSingleStep(0.5);
  endSpinBox_->setMinimum(0.5);
  endSpinBox_->setValue(1);

  autoBox_ = new QCheckBox(tr("Auto"), this);
  autoBox_->setChecked(true);

  // Tooltips
  autoBox_->setToolTip(tr(
    "Use the default value which will try to start from near zero and place"
    " in the middle of the screen the size selected in the selector."));
  startSpinBox_->setToolTip(tr("Start size, will be always guaranteed."));
  endSpinBox_->setToolTip(tr(
    "End size, may not be guaranteed due to rounding and precision issues."));

  // Layouting
  layout_ = new QGridLayout;
  gridLayout2ColAddWidget(layout_, autoBox_);
  gridLayout2ColAddWidget(layout_, startLabel_, startSpinBox_);
  gridLayout2ColAddWidget(layout_, endLabel_, endSpinBox_);

  setLayout(layout_);
}


void
WaterfallConfigDialog::createConnections()
{
  connect(autoBox_, &QCheckBox::clicked,
          this, &WaterfallConfigDialog::checkAutoStatus);
  connect(startSpinBox_,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &WaterfallConfigDialog::sizeUpdated);
  connect(endSpinBox_,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &WaterfallConfigDialog::sizeUpdated);
}


void
WaterfallConfigDialog::checkAutoStatus()
{
  startSpinBox_->setEnabled(!autoBox_->isChecked());
  endSpinBox_->setEnabled(!autoBox_->isChecked());

  emit sizeUpdated();
}


const char* StringSamples[] = {
  "The quick brown fox jumps over the lazy dog",

  /* Luís argüia à Júlia que «brações, fé, chá, óxido, pôr, zângão» */
  /* eram palavras do português */
  "Lu\u00EDs arg\u00FCia \u00E0 J\u00FAlia que \u00ABbra\u00E7\u00F5es,"
  " f\u00E9, ch\u00E1, \u00F3xido, p\u00F4r, z\u00E2ng\u00E3o\u00BB eram"
  " palavras do portugu\u00EAs",

  /* Ο καλύμνιος σφουγγαράς ψιθύρισε πως θα βουτήξει χωρίς να διστάζει */
  "\u039F \u03BA\u03B1\u03BB\u03CD\u03BC\u03BD\u03B9\u03BF\u03C2 \u03C3"
  "\u03C6\u03BF\u03C5\u03B3\u03B3\u03B1\u03C1\u03AC\u03C2 \u03C8\u03B9"
  "\u03B8\u03CD\u03C1\u03B9\u03C3\u03B5 \u03C0\u03C9\u03C2 \u03B8\u03B1"
  " \u03B2\u03BF\u03C5\u03C4\u03AE\u03BE\u03B5\u03B9 \u03C7\u03C9\u03C1"
  "\u03AF\u03C2 \u03BD\u03B1 \u03B4\u03B9\u03C3\u03C4\u03AC\u03B6\u03B5"
  "\u03B9",

  /* Съешь ещё этих мягких французских булок да выпей же чаю */
  "\u0421\u044A\u0435\u0448\u044C \u0435\u0449\u0451 \u044D\u0442\u0438"
  "\u0445 \u043C\u044F\u0433\u043A\u0438\u0445 \u0444\u0440\u0430\u043D"
  "\u0446\u0443\u0437\u0441\u043A\u0438\u0445 \u0431\u0443\u043B\u043E"
  "\u043A \u0434\u0430 \u0432\u044B\u043F\u0435\u0439 \u0436\u0435"
  " \u0447\u0430\u044E",

  /* 天地玄黃，宇宙洪荒。日月盈昃，辰宿列張。寒來暑往，秋收冬藏。*/
  "\u5929\u5730\u7384\u9EC3\uFF0C\u5B87\u5B99\u6D2A\u8352\u3002\u65E5"
  "\u6708\u76C8\u6603\uFF0C\u8FB0\u5BBF\u5217\u5F35\u3002\u5BD2\u4F86"
  "\u6691\u5F80\uFF0C\u79CB\u6536\u51AC\u85CF\u3002",

  /* いろはにほへと ちりぬるを わかよたれそ つねならむ */
  /* うゐのおくやま けふこえて あさきゆめみし ゑひもせす */
  "\u3044\u308D\u306F\u306B\u307B\u3078\u3068 \u3061\u308A\u306C\u308B"
  "\u3092 \u308F\u304B\u3088\u305F\u308C\u305D \u3064\u306D\u306A\u3089"
  "\u3080 \u3046\u3090\u306E\u304A\u304F\u3084\u307E \u3051\u3075\u3053"
  "\u3048\u3066 \u3042\u3055\u304D\u3086\u3081\u307F\u3057 \u3091\u3072"
  "\u3082\u305B\u3059",

  /* 키스의 고유조건은 입술끼리 만나야 하고 특별한 기술은 필요치 않다 */
  "\uD0A4\uC2A4\uC758 \uACE0\uC720\uC870\uAC74\uC740 \uC785\uC220\uB07C"
  "\uB9AC \uB9CC\uB098\uC57C \uD558\uACE0 \uD2B9\uBCC4\uD55C \uAE30"
  "\uC220\uC740 \uD544\uC694\uCE58 \uC54A\uB2E4"
};


// end of continuous.cpp
