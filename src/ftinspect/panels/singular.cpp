// singular.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "singular.hpp"

#include <QSizePolicy>
#include <QWheelEvent>


SingularTab::SingularTab(QWidget* parent, Engine* engine)
: QWidget(parent), engine_(engine),
  graphicsDefault_(GraphicsDefault::deafultInstance())
{
  createLayout();
  createConnections();

  currentGlyphIndex_ = 0;
  checkShowPoints();
}


SingularTab::~SingularTab()
{
  delete gridItem_;
  gridItem_ = NULL;
}


void
SingularTab::setGlyphIndex(int index)
{
  // only adjust current glyph index if we have a valid font
  if (currentGlyphCount_ > 0)
  {
    currentGlyphIndex_ = qBound(0, index, currentGlyphCount_ - 1);
  }

  QString upperHex = QString::number(currentGlyphIndex_, 16).toUpper();
  glyphIndexLabel_->setText(
      QString("%1 (0x%2)").arg(currentGlyphIndex_).arg(upperHex));
  glyphNameLabel_->setText(engine_->glyphName(currentGlyphIndex_));

  drawGlyph();
}


void
SingularTab::drawGlyph()
{
  // the call to `engine->loadOutline' updates FreeType's load flags

  if (!engine_)
    return;

  if (currentGlyphBitmapItem_)
  {
    glyphScene_->removeItem(currentGlyphBitmapItem_);
    delete currentGlyphBitmapItem_;

    currentGlyphBitmapItem_ = NULL;
  }

  if (currentGlyphOutlineItem_)
  {
    glyphScene_->removeItem(currentGlyphOutlineItem_);
    delete currentGlyphOutlineItem_;

    currentGlyphOutlineItem_ = NULL;
  }

  if (currentGlyphPointsItem_)
  {
    glyphScene_->removeItem(currentGlyphPointsItem_);
    delete currentGlyphPointsItem_;

    currentGlyphPointsItem_ = NULL;
  }

  if (currentGlyphPointNumbersItem_)
  {
    glyphScene_->removeItem(currentGlyphPointNumbersItem_);
    delete currentGlyphPointNumbersItem_;

    currentGlyphPointNumbersItem_ = NULL;
  }

  syncSettings();
  FT_Outline* outline = engine_->loadOutline(currentGlyphIndex_);
  if (outline)
  {
    if (showBitmapCheckBox_->isChecked())
    {
      // XXX support LCD
      FT_Pixel_Mode pixelMode = FT_PIXEL_MODE_GRAY;
      if (!engine_->antiAliasingEnabled())
        pixelMode = FT_PIXEL_MODE_MONO;

      currentGlyphBitmapItem_
        = new GlyphBitmap(outline,
          engine_->ftLibrary(),
          pixelMode,
          graphicsDefault_->monoColorTable,
          graphicsDefault_->grayColorTable);
      glyphScene_->addItem(currentGlyphBitmapItem_);
    }

    if (showOutlinesCheckBox_->isChecked())
    {
      currentGlyphOutlineItem_ = new GlyphOutline(graphicsDefault_->outlinePen, 
                                                  outline);
      glyphScene_->addItem(currentGlyphOutlineItem_);
    }

    if (showPointsCheckBox_->isChecked())
    {
      currentGlyphPointsItem_ = new GlyphPoints(graphicsDefault_->onPen,
                                                graphicsDefault_->offPen,
                                                outline);
      glyphScene_->addItem(currentGlyphPointsItem_);

      if (showPointNumbersCheckBox_->isChecked())
      {
        currentGlyphPointNumbersItem_
          = new GlyphPointNumbers(graphicsDefault_->onPen,
                                  graphicsDefault_->offPen,
                                  outline);
        glyphScene_->addItem(currentGlyphPointNumbersItem_);
      }
    }
  }

  glyphScene_->update();
}


void
SingularTab::checkShowPoints()
{
  if (showPointsCheckBox_->isChecked())
    showPointNumbersCheckBox_->setEnabled(true);
  else
    showPointNumbersCheckBox_->setEnabled(false);
  drawGlyph();
}


void
SingularTab::zoom()
{
  int scale = zoomSpinBox_->value();

  QTransform transform;
  transform.scale(scale, scale);

  // we want horizontal and vertical 1px lines displayed with full pixels;
  // we thus have to shift the coordinate system accordingly, using a value
  // that represents 0.5px (i.e., half the 1px line width) after the scaling
  qreal shift = 0.5 / scale;
  transform.translate(shift, shift);

  glyphView_->setTransform(transform);
  updateGrid();
}


void
SingularTab::backToCenter()
{
  glyphView_->centerOn(0, 0);
  if (currentGlyphBitmapItem_)
    glyphView_->ensureVisible(currentGlyphBitmapItem_);
  else if (currentGlyphPointsItem_)
    glyphView_->ensureVisible(currentGlyphPointsItem_);

  updateGrid();
}


void
SingularTab::updateGrid()
{
  if (gridItem_)
    gridItem_->updateRect();
}


void
SingularTab::wheelZoom(QWheelEvent* event)
{
  int numSteps = event->angleDelta().y() / 120;
  int zoomAfter = zoomSpinBox_->value() + numSteps;
  zoomAfter = std::max(zoomSpinBox_->minimum(),
                       std::min(zoomAfter, zoomSpinBox_->maximum()));
  zoomSpinBox_->setValue(zoomAfter);
  // TODO: Zoom relative to viewport left-bottom?
}


void
SingularTab::wheelResize(QWheelEvent* event)
{
  sizeSelector_->handleWheelResizeFromGrid(event);
}


void
SingularTab::createLayout()
{
  glyphScene_ = new QGraphicsScene(this);

  currentGlyphBitmapItem_ = NULL;
  currentGlyphOutlineItem_ = NULL;
  currentGlyphPointsItem_ = NULL;
  currentGlyphPointNumbersItem_ = NULL;

  glyphView_ = new QGraphicsViewx(this);
  glyphView_->setRenderHint(QPainter::Antialiasing, true);
  glyphView_->setAcceptDrops(false);
  glyphView_->setDragMode(QGraphicsView::ScrollHandDrag);
  glyphView_->setOptimizationFlags(QGraphicsView::DontSavePainterState);
  glyphView_->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
  glyphView_->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  glyphView_->setScene(glyphScene_);

  gridItem_ = new Grid(glyphView_, graphicsDefault_->gridPen, 
                       graphicsDefault_->axisPen);
  glyphScene_->addItem(gridItem_);

  // Don't use QGraphicsTextItem: We want this hint to be anchored at the
  // top-left corner.
  mouseUsageHint_ = new QLabel(tr(
                      "Scroll: Grid Up/Down\n"
                      "Alt + Scroll: Grid Left/Right\n"
                      "Ctrl + Scroll: Adjust Zoom (Relative to cursor)\n"
                      "Shift + Scroll: Adjust Font Size"),
                      glyphView_);
  auto hintFont = font();
  hintFont.setPixelSize(24);
  mouseUsageHint_->setFont(hintFont);
  mouseUsageHint_->setAttribute(Qt::WA_TransparentForMouseEvents, true);

  glyphIndexLabel_ = new QLabel(glyphView_);
  glyphNameLabel_ = new QLabel(glyphView_);
  glyphIndexLabel_->setFont(hintFont);
  glyphNameLabel_->setFont(hintFont);
  glyphIndexLabel_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  glyphNameLabel_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  glyphIndexLabel_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  glyphNameLabel_->setAttribute(Qt::WA_TransparentForMouseEvents, true);

  indexSelector_ = new GlyphIndexSelector(this);
  indexSelector_->setSingleMode(true);

  sizeSelector_ = new FontSizeSelector(this);

  zoomLabel_ = new QLabel(tr("Zoom Factor"), this);
  zoomLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  zoomSpinBox_ = new ZoomSpinBox(this);
  zoomSpinBox_->setAlignment(Qt::AlignRight);
  zoomSpinBox_->setRange(1, 1000 - 1000 % 64);
  zoomSpinBox_->setKeyboardTracking(false);
  zoomLabel_->setBuddy(zoomSpinBox_);

  centerGridButton_ = new QPushButton("Go Back to Grid Center", this);

  showBitmapCheckBox_ = new QCheckBox(tr("Show Bitmap"), this);
  showPointsCheckBox_ = new QCheckBox(tr("Show Points"), this);
  showPointNumbersCheckBox_ = new QCheckBox(tr("Show Point Numbers"), this);
  showOutlinesCheckBox_ = new QCheckBox(tr("Show Outlines"), this);

  sizeLayout_ = new QHBoxLayout;
  sizeLayout_->addStretch(2);
  sizeLayout_->addWidget(sizeSelector_, 3);
  sizeLayout_->addStretch(1);
  sizeLayout_->addWidget(zoomLabel_);
  sizeLayout_->addWidget(zoomSpinBox_);
  sizeLayout_->addStretch(1);
  sizeLayout_->addWidget(centerGridButton_);
  sizeLayout_->addStretch(2);

  checkBoxesLayout_ = new QHBoxLayout;
  checkBoxesLayout_->setSpacing(10);
  checkBoxesLayout_->addWidget(showBitmapCheckBox_);
  checkBoxesLayout_->addWidget(showPointsCheckBox_);
  checkBoxesLayout_->addWidget(showPointNumbersCheckBox_);
  checkBoxesLayout_->addWidget(showOutlinesCheckBox_);

  glyphOverlayIndexLayout_ = new QHBoxLayout;
  glyphOverlayIndexLayout_->addWidget(glyphIndexLabel_);
  glyphOverlayIndexLayout_->addWidget(glyphNameLabel_);
  glyphOverlayLayout_ = new QGridLayout;
  glyphOverlayLayout_->addWidget(mouseUsageHint_, 0, 0,
                                 Qt::AlignTop | Qt::AlignLeft);
  glyphOverlayLayout_->addLayout(glyphOverlayIndexLayout_, 0, 1,
                                 Qt::AlignTop | Qt::AlignRight);
  glyphView_->setLayout(glyphOverlayLayout_);

  mainLayout_ = new QVBoxLayout;
  mainLayout_->addWidget(glyphView_);
  mainLayout_->addWidget(indexSelector_);
  mainLayout_->addSpacing(10); // XXX px
  mainLayout_->addLayout(sizeLayout_);
  mainLayout_->addLayout(checkBoxesLayout_);
  mainLayout_->addSpacing(10); // XXX px

  setLayout(mainLayout_);
}


void
SingularTab::createConnections()
{
  connect(sizeSelector_, &FontSizeSelector::valueChanged,
          this, &SingularTab::repaintGlyph);
  connect(indexSelector_, &GlyphIndexSelector::currentIndexChanged, 
          this, &SingularTab::setGlyphIndex);

  connect(zoomSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged),
          this, &SingularTab::zoom);
  connect(glyphView_, &QGraphicsViewx::shiftWheelEvent, 
          this, &SingularTab::wheelResize);
  connect(glyphView_, &QGraphicsViewx::ctrlWheelEvent, 
          this, &SingularTab::wheelZoom);
  connect(glyphView_->horizontalScrollBar(), &QScrollBar::valueChanged,
          this, &SingularTab::updateGrid);
  connect(glyphView_->verticalScrollBar(), &QScrollBar::valueChanged, 
          this, &SingularTab::updateGrid);

  connect(centerGridButton_, &QPushButton::clicked,
          this, &SingularTab::backToCenter);

  connect(showBitmapCheckBox_, &QCheckBox::clicked,
          this, &SingularTab::drawGlyph);
  connect(showPointsCheckBox_, &QCheckBox::clicked, 
          this, &SingularTab::checkShowPoints);
  connect(showPointNumbersCheckBox_, &QCheckBox::clicked,
          this, &SingularTab::drawGlyph);
  connect(showOutlinesCheckBox_, &QCheckBox::clicked,
          this, &SingularTab::drawGlyph);
}


void
SingularTab::repaintGlyph()
{
  drawGlyph();
}


void
SingularTab::reloadFont()
{
  currentGlyphCount_ = engine_->currentFontNumberOfGlyphs();
  indexSelector_->setMinMax(0, currentGlyphCount_);
  drawGlyph();
}


void
SingularTab::syncSettings()
{
  sizeSelector_->applyToEngine(engine_);
}


void
SingularTab::setDefaults()
{
  currentGlyphIndex_ = 0;

  zoomSpinBox_->setValue(20);
  showBitmapCheckBox_->setChecked(true);
  showOutlinesCheckBox_->setChecked(true);
  
  indexSelector_->setCurrentIndex(indexSelector_->currentIndex(), true);
  zoom();
}


// end of singular.cpp
