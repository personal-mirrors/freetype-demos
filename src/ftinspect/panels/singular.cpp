// singular.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "singular.hpp"

#include <QSizePolicy>
#include <QToolTip>
#include <QWheelEvent>


SingularTab::SingularTab(QWidget* parent,
                         Engine* engine)
: QWidget(parent),
  engine_(engine),
  graphicsDefault_(GraphicsDefault::deafultInstance())
{
  createLayout();
  createConnections();

  currentGlyphIndex_ = 0;
  setDefaults();
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
  // Only adjust current glyph index if we have a valid font.
  if (currentGlyphCount_ <= 0)
    return;

  currentGlyphIndex_ = qBound(0, index, currentGlyphCount_ - 1);

  QString upperHex = QString::number(currentGlyphIndex_, 16).toUpper();
  glyphIndexLabel_->setText(
    QString("%1 (0x%2)").arg(currentGlyphIndex_).arg(upperHex));
  glyphNameLabel_->setText(engine_->glyphName(currentGlyphIndex_));

  drawGlyph();
}


void
SingularTab::drawGlyph()
{
  // The call to `engine->loadOutline' updates FreeType's load flags.

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

  glyphView_->setBackgroundBrush(
    QColor(engine_->renderingEngine()->background()));

  applySettings();
  engine_->loadPalette();
  FT_Glyph glyph = engine_->loadGlyph(currentGlyphIndex_);
  if (glyph)
  {
    if (showBitmapCheckBox_->isChecked())
    {
      currentGlyphBitmapItem_ = new GlyphBitmap(currentGlyphIndex_,
                                                glyph,
                                                engine_);
      currentGlyphBitmapItem_->setZValue(-1);
      glyphScene_->addItem(currentGlyphBitmapItem_);
    }

    if (showOutlinesCheckBox_->isChecked())
    {
      currentGlyphOutlineItem_ =
        new GlyphOutline(graphicsDefault_->outlinePen, glyph);
      currentGlyphOutlineItem_->setZValue(1);
      glyphScene_->addItem(currentGlyphOutlineItem_);
    }

    if (showPointsCheckBox_->isChecked())
    {
      currentGlyphPointsItem_ = new GlyphPoints(engine_->ftLibrary(),
                                                graphicsDefault_->onPen,
                                                graphicsDefault_->offPen,
                                                glyph);
      currentGlyphPointsItem_->setZValue(1);
      glyphScene_->addItem(currentGlyphPointsItem_);

      if (showPointNumbersCheckBox_->isChecked())
      {
        currentGlyphPointNumbersItem_
          = new GlyphPointNumbers(engine_->ftLibrary(),
                                  graphicsDefault_->onPen,
                                  graphicsDefault_->offPen,
                                  glyph);
        currentGlyphPointNumbersItem_->setZValue(1);
        glyphScene_->addItem(currentGlyphPointNumbersItem_);
      }
    }

    engine_->reloadFont();
    auto ascDesc = engine_->currentSizeAscDescPx();
    gridItem_->updateParameters(ascDesc.first, ascDesc.second,
                                glyph->advance.x >> 16);
  }
  else
    gridItem_->updateParameters(0, 0, 0);

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
  int scale = static_cast<int>(sizeSelector_->zoomFactor());

  QTransform transform;
  transform.scale(scale, scale);

  // We want horizontal and vertical 1px lines displayed with full pixels;
  // we thus have to shift the coordinate system accordingly, using a value
  // that represents 0.5px (i.e., half the 1px line width) after the scaling.
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
  sizeSelector_->handleWheelZoomBySteps(numSteps);
  // TODO: Zoom relative to viewport left-bottom?
}


void
SingularTab::wheelResize(QWheelEvent* event)
{
  sizeSelector_->handleWheelResizeFromGrid(event);
}


void
SingularTab::setGridVisible()
{
  gridItem_->setShowGrid(showGridCheckBox_->isChecked(),
                         showAuxLinesCheckBox_->isChecked());
}


void
SingularTab::showToolTip()
{
  QToolTip::showText(mapToGlobal(helpButton_->pos()),
                     tr("Scroll: Grid Up/Down\n"
                        "Alt + Scroll: Grid Left/Right\n"
                        "Ctrl + Scroll: Adjust Zoom (Relative to cursor)\n"
                        "Shift + Scroll: Adjust Font Size\n"
                        "Shift + Plus/Minus: Adjust Font Size\n"
                        "Shift + 0: Reset Font Size to Default"),
                     helpButton_);
}


bool
SingularTab::eventFilter(QObject* watched,
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
SingularTab::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);

  // Tricky part: when loading, this method will be called twice.  Only at the
  // second time the initial layouting is done, thus the result of `centerOn`
  // can be valid.
  // We want to only center the midpoint of the size when the program starts up
  // so we use a counter to track the status.
  if (initialPositionSetCount_ <= 0)
    return;
  initialPositionSetCount_--;

  // The code below does mainly the following:
  // 1. Center the grid on the the center point of the x ppem bbox.
  // 2. Adjust the viewport zoom to fit the x ppem bbox
  updateGeometry();
  auto size = sizeSelector_->selectedSize();
  auto unit = sizeSelector_->selectedUnit();
  if (unit == FontSizeSelector::Units_pt)
  {
    sizeSelector_->applyToEngine(engine_);
    auto dpi = engine_->dpi();
    size = size * dpi / 72.0;
  }
  glyphView_->centerOn(size / 2, -size / 2);

  auto viewSize = glyphView_->size();
  auto minViewSide = std::min(viewSize.height(), viewSize.width());
  sizeSelector_->setZoomFactor(static_cast<int>(minViewSide / size * 0.7));
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
  glyphView_->setBackgroundBrush(Qt::white);

  gridItem_ = new Grid(glyphView_);
  glyphScene_->addItem(gridItem_);

  // Don't use `QGraphicsTextItem`: We want this hint to be anchored at the
  // top-left corner.
  auto overlayFont = font();
  overlayFont.setPixelSize(24);

  glyphIndexLabel_ = new QLabel(glyphView_);
  glyphNameLabel_ = new QLabel(glyphView_);
  glyphIndexLabel_->setFont(overlayFont);
  glyphNameLabel_->setFont(overlayFont);
  glyphIndexLabel_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  glyphNameLabel_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  glyphIndexLabel_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  glyphNameLabel_->setAttribute(Qt::WA_TransparentForMouseEvents, true);

  glyphIndexLabel_->setStyleSheet("QLabel { color : black; }");
  glyphNameLabel_->setStyleSheet("QLabel { color : black; }");

  indexSelector_ = new GlyphIndexSelector(this);
  indexSelector_->setSingleMode(true);

  sizeSelector_ = new FontSizeSelector(this, false, false);

  centerGridButton_ = new QPushButton("Go Back to Grid Center", this);
  helpButton_ = new QPushButton("?", this);
  helpButton_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  showBitmapCheckBox_ = new QCheckBox(tr("Show Bitmap"), this);
  showPointsCheckBox_ = new QCheckBox(tr("Show Points"), this);
  showPointNumbersCheckBox_ = new QCheckBox(tr("Show Point Numbers"), this);
  showOutlinesCheckBox_ = new QCheckBox(tr("Show Outlines"), this);
  showGridCheckBox_ = new QCheckBox(tr("Show Grid"), this);
  showAuxLinesCheckBox_ = new QCheckBox(tr("Show Aux. Lines"), this);

  // Tooltips
  centerGridButton_->setToolTip(tr(
    "Move the viewport so the origin point is at the center of the view."));
  showBitmapCheckBox_->setToolTip(tr("Show glyph bitmap."));
  showPointsCheckBox_->setToolTip(tr(
    "Show control points (only valid when the glyph is an outline glyph)."));
  showPointNumbersCheckBox_->setToolTip(tr(
    "Show point numbers (only available when points are shown)."));
  showOutlinesCheckBox_->setToolTip(tr(
    "Show (vector) outline (only valid when the glyph is an outline glyph)."));
  showGridCheckBox_->setToolTip(tr("Show grid lines (x axis: baseline)."));
  showBitmapCheckBox_->setToolTip(tr(
    "Show auxiliary lines (blue: y-advance; red: ascender/descender)."));
  helpButton_->setToolTip(tr("View scroll help"));

  // Layouting
  indexHelpLayout_ = new QHBoxLayout;
  indexHelpLayout_->addWidget(indexSelector_, 1);
  indexHelpLayout_->addWidget(helpButton_);

  sizeLayout_ = new QHBoxLayout;
  sizeLayout_->addStretch(2);
  sizeLayout_->addWidget(sizeSelector_, 4);
  sizeLayout_->addStretch(1);
  sizeLayout_->addWidget(centerGridButton_);
  sizeLayout_->addStretch(2);

  checkBoxesLayout_ = new QHBoxLayout;
  checkBoxesLayout_->setSpacing(10);
  checkBoxesLayout_->addWidget(showBitmapCheckBox_);
  checkBoxesLayout_->addWidget(showPointsCheckBox_);
  checkBoxesLayout_->addWidget(showPointNumbersCheckBox_);
  checkBoxesLayout_->addWidget(showOutlinesCheckBox_);
  checkBoxesLayout_->addWidget(showGridCheckBox_);
  checkBoxesLayout_->addWidget(showAuxLinesCheckBox_);

  glyphOverlayIndexLayout_ = new QHBoxLayout;
  glyphOverlayIndexLayout_->addWidget(glyphIndexLabel_);
  glyphOverlayIndexLayout_->addWidget(glyphNameLabel_);
  glyphOverlayLayout_ = new QGridLayout; // Use a grid layout to align.
  glyphOverlayLayout_->addLayout(glyphOverlayIndexLayout_, 0, 1,
                                 Qt::AlignTop | Qt::AlignRight);
  glyphView_->setLayout(glyphOverlayLayout_);

  mainLayout_ = new QVBoxLayout;
  mainLayout_->addWidget(glyphView_);
  mainLayout_->addLayout(indexHelpLayout_);
  mainLayout_->addSpacing(10);
  mainLayout_->addLayout(sizeLayout_);
  mainLayout_->addLayout(checkBoxesLayout_);
  mainLayout_->addSpacing(10);

  setLayout(mainLayout_);
}


void
SingularTab::createConnections()
{
  connect(sizeSelector_, &FontSizeSelector::valueChanged,
          this, &SingularTab::repaintGlyph);
  connect(indexSelector_, &GlyphIndexSelector::currentIndexChanged,
          this, &SingularTab::setGlyphIndex);

  connect(glyphView_, &QGraphicsViewx::shiftWheelEvent,
          this, &SingularTab::wheelResize);
  connect(glyphView_, &QGraphicsViewx::ctrlWheelEvent,
          this, &SingularTab::wheelZoom);
  // Use `updateGrid` to support infinite panning.
  connect(glyphView_->horizontalScrollBar(), &QScrollBar::valueChanged,
          this, &SingularTab::updateGrid);
  connect(glyphView_->verticalScrollBar(), &QScrollBar::valueChanged,
          this, &SingularTab::updateGrid);

  connect(centerGridButton_, &QPushButton::clicked,
          this, &SingularTab::backToCenter);
  connect(helpButton_, &QPushButton::clicked,
          this, &SingularTab::showToolTip);

  connect(showBitmapCheckBox_, &QCheckBox::clicked,
          this, &SingularTab::drawGlyph);
  connect(showPointsCheckBox_, &QCheckBox::clicked,
          this, &SingularTab::checkShowPoints);
  connect(showPointNumbersCheckBox_, &QCheckBox::clicked,
          this, &SingularTab::drawGlyph);
  connect(showOutlinesCheckBox_, &QCheckBox::clicked,
          this, &SingularTab::drawGlyph);
  connect(showGridCheckBox_, &QCheckBox::clicked,
          this, &SingularTab::setGridVisible);
  connect(showAuxLinesCheckBox_, &QCheckBox::clicked,
          this, &SingularTab::setGridVisible);

  sizeSelector_->installEventFilterForWidget(glyphView_);
  sizeSelector_->installEventFilterForWidget(this);
}


void
SingularTab::repaintGlyph()
{
  zoom();
  drawGlyph();
}


void
SingularTab::reloadFont()
{
  currentGlyphCount_ = engine_->currentFontNumberOfGlyphs();
  indexSelector_->setMinMax(0, currentGlyphCount_);
  {
    QSignalBlocker blocker(sizeSelector_);
    sizeSelector_->reloadFromFont(engine_);
  }
  drawGlyph();
}


void
SingularTab::setCurrentGlyphAndSize(int glyphIndex,
                                    double sizePoint)
{
  if (sizePoint >= 0)
    sizeSelector_->setSizePoint(sizePoint);
  indexSelector_->setCurrentIndex(glyphIndex); // This auto-triggers update.
}


int
SingularTab::currentGlyph()
{
  return indexSelector_->currentIndex();
}


void
SingularTab::applySettings()
{
  sizeSelector_->applyToEngine(engine_);
}


void
SingularTab::setDefaults()
{
  currentGlyphIndex_ = 0;

  showBitmapCheckBox_->setChecked(true);
  showOutlinesCheckBox_->setChecked(true);
  showGridCheckBox_->setChecked(true);
  showAuxLinesCheckBox_->setChecked(true);
  gridItem_->setShowGrid(true, true);

  indexSelector_->setCurrentIndex(indexSelector_->currentIndex(), true);
  zoom();
}


// end of singular.cpp
