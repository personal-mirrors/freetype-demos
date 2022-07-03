// maingui.cpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#include "maingui.hpp"
#include "rendering/grid.hpp"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

#include <freetype/ftdriver.h>


MainGUI::MainGUI(Engine* engine)
: engine_(engine)
{
  setGraphicsDefaults();
  createLayout();
  createConnections();
  createActions();
  createMenus();
  createStatusBar();

  readSettings();

  setUnifiedTitleAndToolBarOnMac(true);
}


MainGUI::~MainGUI()
{
  // empty
}


// overloading

void
MainGUI::closeEvent(QCloseEvent* event)
{
  writeSettings();
  event->accept();
}


void
MainGUI::about()
{
  QMessageBox::about(
    this,
    tr("About ftinspect"),
    tr("<p>This is <b>ftinspect</b> version %1<br>"
       " Copyright %2 2016-2022<br>"
       " by Werner Lemberg <tt>&lt;wl@gnu.org&gt;</tt></p>"
       ""
       "<p><b>ftinspect</b> shows how a font gets rendered"
       " by FreeType, allowing control over virtually"
       " all rendering parameters.</p>"
       ""
       "<p>License:"
       " <a href='https://gitlab.freedesktop.org/freetype/freetype/-/blob/master/docs/FTL.TXT'>FreeType"
       " License (FTL)</a> or"
       " <a href='https://gitlab.freedesktop.org/freetype/freetype/-/blob/master/docs/GPLv2.TXT'>GNU"
       " GPLv2</a></p>")
       .arg(QApplication::applicationVersion())
       .arg(QChar(0xA9)));
}


void
MainGUI::aboutQt()
{
  QApplication::aboutQt();
}


void
MainGUI::loadFonts()
{
  int oldSize = engine_->numberOfOpenedFonts();

  QStringList files = QFileDialog::getOpenFileNames(
                        this,
                        tr("Load one or more fonts"),
                        QDir::homePath(),
                        "",
                        NULL,
                        QFileDialog::ReadOnly);

  engine_->openFonts(files);

  // if we have new fonts, set the current index to the first new one
  if (oldSize < engine_->numberOfOpenedFonts())
    currentFontIndex_ = oldSize;

  showFont();
}


void
MainGUI::closeFont()
{
  if (currentFontIndex_ < engine_->numberOfOpenedFonts())
  {
    engine_->removeFont(currentFontIndex_);
  }

  // show next font after deletion, i.e., retain index if possible
  int num = engine_->numberOfOpenedFonts();
  if (num)
  {
    if (currentFontIndex_ >= num)
      currentFontIndex_ = num - 1;
  }
  else
    currentFontIndex_ = 0;

  showFont();
}


void
MainGUI::watchCurrentFont()
{
  showFont();
}


void
MainGUI::showFont()
{
  // we do lazy computation of FT_Face objects

  if (currentFontIndex_ < engine_->numberOfOpenedFonts())
  {
    QFileInfo& fileInfo = engine_->fontFileManager()[currentFontIndex_];
    QString fontName = fileInfo.fileName();

    engine_->fontFileManager().updateWatching(currentFontIndex_);
    if (fileInfo.isSymLink())
    {
      fontName.prepend("<i>");
      fontName.append("</i>");
    }

    if (!fileInfo.exists())
    {
      // On Unix-like systems, the symlink's target gets opened; this
      // implies that deletion of a symlink doesn't make `engine->loadFont'
      // fail since it operates on a file handle pointing to the target.
      // For this reason, we remove the font to enforce a reload.
      engine_->removeFont(currentFontIndex_, false);
    }

    fontFilenameLabel_->setText(fontName);
  }
  else
    fontFilenameLabel_->clear();

  syncSettings();
  currentNumberOfFaces_
    = engine_->numberOfFaces(currentFontIndex_);
  currentNumberOfNamedInstances_
    = engine_->numberOfNamedInstances(currentFontIndex_,
                                     currentFaceIndex_);
  currentNumberOfGlyphs_
    = engine_->loadFont(currentFontIndex_,
                       currentFaceIndex_,
                       currentNamedInstanceIndex_);

  if (currentNumberOfGlyphs_ < 0)
  {
    // there might be various reasons why the current
    // (file, face, instance) triplet is invalid or missing;
    // we thus start our timer to periodically test
    // whether the font starts working
    if (currentFontIndex_ > 0
        && currentFontIndex_ < engine_->numberOfOpenedFonts())
      engine_->fontFileManager().timerStart();
  }

  fontNameLabel_->setText(QString("%1 %2")
                         .arg(engine_->currentFamilyName())
                         .arg(engine_->currentStyleName()));

  checkCurrentFontIndex();
  checkCurrentFaceIndex();
  checkCurrentNamedInstanceIndex();
  auto state = settingPanel_->blockSignals(true);
  settingPanel_->checkHinting();
  settingPanel_->blockSignals(state);
  adjustGlyphIndex(0);
}


void
MainGUI::syncSettings()
{
  // Spinbox value cannot become negative
  engine_->setDPI(static_cast<unsigned int>(dpiSpinBox_->value()));

  if (unitsComboBox_->currentIndex() == Units_px)
    engine_->setSizeByPixel(sizeDoubleSpinBox_->value());
  else
    engine_->setSizeByPoint(sizeDoubleSpinBox_->value());

  settingPanel_->syncSettings();
}


void
MainGUI::clearStatusBar()
{
  statusBar()->clearMessage();
  statusBar()->setStyleSheet("");
}


void
MainGUI::checkUnits()
{
  int index = unitsComboBox_->currentIndex();

  if (index == Units_px)
  {
    dpiLabel_->setEnabled(false);
    dpiSpinBox_->setEnabled(false);
    sizeDoubleSpinBox_->setSingleStep(1);
    sizeDoubleSpinBox_->setValue(qRound(sizeDoubleSpinBox_->value()));
  }
  else
  {
    dpiLabel_->setEnabled(true);
    dpiSpinBox_->setEnabled(true);
    sizeDoubleSpinBox_->setSingleStep(0.5);
  }

  drawGlyph();
}


void
MainGUI::adjustGlyphIndex(int delta)
{
  // only adjust current glyph index if we have a valid font
  if (currentNumberOfGlyphs_ > 0)
  {
    currentGlyphIndex_ += delta;
    currentGlyphIndex_ = qBound(0,
                               currentGlyphIndex_,
                               currentNumberOfGlyphs_ - 1);
  }

  QString upperHex = QString::number(currentGlyphIndex_, 16).toUpper();
  glyphIndexLabel_->setText(QString("%1 (0x%2)")
                                   .arg(currentGlyphIndex_)
                                   .arg(upperHex));
  glyphNameLabel_->setText(engine_->glyphName(currentGlyphIndex_));

  drawGlyph();
}


void
MainGUI::checkCurrentFontIndex()
{
  if (engine_->numberOfOpenedFonts() < 2)
  {
    previousFontButton_->setEnabled(false);
    nextFontButton_->setEnabled(false);
  }
  else if (currentFontIndex_ == 0)
  {
    previousFontButton_->setEnabled(false);
    nextFontButton_->setEnabled(true);
  }
  else if (currentFontIndex_ >= engine_->numberOfOpenedFonts() - 1)
  {
    previousFontButton_->setEnabled(true);
    nextFontButton_->setEnabled(false);
  }
  else
  {
    previousFontButton_->setEnabled(true);
    nextFontButton_->setEnabled(true);
  }
}


void
MainGUI::checkCurrentFaceIndex()
{
  if (currentNumberOfFaces_ < 2)
  {
    previousFaceButton_->setEnabled(false);
    nextFaceButton_->setEnabled(false);
  }
  else if (currentFaceIndex_ == 0)
  {
    previousFaceButton_->setEnabled(false);
    nextFaceButton_->setEnabled(true);
  }
  else if (currentFaceIndex_ >= currentNumberOfFaces_ - 1)
  {
    previousFaceButton_->setEnabled(true);
    nextFaceButton_->setEnabled(false);
  }
  else
  {
    previousFaceButton_->setEnabled(true);
    nextFaceButton_->setEnabled(true);
  }
}


void
MainGUI::checkCurrentNamedInstanceIndex()
{
  if (currentNumberOfNamedInstances_ < 2)
  {
    previousNamedInstanceButton_->setEnabled(false);
    nextNamedInstanceButton_->setEnabled(false);
  }
  else if (currentNamedInstanceIndex_ == 0)
  {
    previousNamedInstanceButton_->setEnabled(false);
    nextNamedInstanceButton_->setEnabled(true);
  }
  else if (currentNamedInstanceIndex_ >= currentNumberOfNamedInstances_ - 1)
  {
    previousNamedInstanceButton_->setEnabled(true);
    nextNamedInstanceButton_->setEnabled(false);
  }
  else
  {
    previousNamedInstanceButton_->setEnabled(true);
    nextNamedInstanceButton_->setEnabled(true);
  }
}


void
MainGUI::previousFont()
{
  if (currentFontIndex_ > 0)
  {
    currentFontIndex_--;
    currentFaceIndex_ = 0;
    currentNamedInstanceIndex_ = 0;
    showFont();
  }
}


void
MainGUI::nextFont()
{
  if (currentFontIndex_ < engine_->numberOfOpenedFonts() - 1)
  {
    currentFontIndex_++;
    currentFaceIndex_ = 0;
    currentNamedInstanceIndex_ = 0;
    showFont();
  }
}


void
MainGUI::previousFace()
{
  if (currentFaceIndex_ > 0)
  {
    currentFaceIndex_--;
    currentNamedInstanceIndex_ = 0;
    showFont();
  }
}


void
MainGUI::nextFace()
{
  if (currentFaceIndex_ < currentNumberOfFaces_ - 1)
  {
    currentFaceIndex_++;
    currentNamedInstanceIndex_ = 0;
    showFont();
  }
}


void
MainGUI::previousNamedInstance()
{
  if (currentNamedInstanceIndex_ > 0)
  {
    currentNamedInstanceIndex_--;
    showFont();
  }
}


void
MainGUI::nextNamedInstance()
{
  if (currentNamedInstanceIndex_ < currentNumberOfNamedInstances_ - 1)
  {
    currentNamedInstanceIndex_++;
    showFont();
  }
}


void
MainGUI::zoom()
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
}


void
MainGUI::backToCenter()
{
  glyphView_->centerOn(0, 0);
  if (currentGlyphBitmapItem_)
    glyphView_->ensureVisible(currentGlyphBitmapItem_);
  else if (currentGlyphPointsItem_)
    glyphView_->ensureVisible(currentGlyphPointsItem_);
}


void
MainGUI::wheelZoom(QWheelEvent* event)
{
  int numSteps = event->angleDelta().y() / 120;
  int zoomAfter = zoomSpinBox_->value() + numSteps;
  zoomAfter = std::max(zoomSpinBox_->minimum(),
                       std::min(zoomAfter, zoomSpinBox_->maximum()));
  zoomSpinBox_->setValue(zoomAfter);
  // TODO: Zoom relative to viewport left-bottom?
}


void
MainGUI::wheelResize(QWheelEvent* event)
{
  int numSteps = event->angleDelta().y() / 120;
  double sizeAfter = sizeDoubleSpinBox_->value() + numSteps * 0.5;
  sizeAfter = std::max(sizeDoubleSpinBox_->minimum(),
                       std::min(sizeAfter, sizeDoubleSpinBox_->maximum()));
  sizeDoubleSpinBox_->setValue(sizeAfter);
}


void
MainGUI::setGraphicsDefaults()
{
  // color tables (with suitable opacity values) for converting
  // FreeType's pixmaps to something Qt understands
  monoColorTable_.append(QColor(Qt::transparent).rgba());
  monoColorTable_.append(QColor(Qt::black).rgba());

  for (int i = 0xFF; i >= 0; i--)
    grayColorTable_.append(qRgba(i, i, i, 0xFF - i));

  // XXX make this user-configurable

  axisPen_.setColor(Qt::black);
  axisPen_.setWidth(0);
  blueZonePen_.setColor(QColor(64, 64, 255, 64)); // light blue
  blueZonePen_.setWidth(0);
  gridPen_.setColor(Qt::lightGray);
  gridPen_.setWidth(0);
  offPen_.setColor(Qt::darkGreen);
  offPen_.setWidth(3);
  onPen_.setColor(Qt::red);
  onPen_.setWidth(3);
  outlinePen_.setColor(Qt::red);
  outlinePen_.setWidth(0);
  segmentPen_.setColor(QColor(64, 255, 128, 64)); // light green
  segmentPen_.setWidth(0);
}


void
MainGUI::drawGlyph()
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
    if (settingPanel_->showBitmapChecked())
    {
      // XXX support LCD
      FT_Pixel_Mode pixelMode = FT_PIXEL_MODE_GRAY;
      if (settingPanel_->antiAliasingModeIndex()
          == AntiAliasingComboBoxModel::AntiAliasing_None)
        pixelMode = FT_PIXEL_MODE_MONO;

      currentGlyphBitmapItem_ = new GlyphBitmap(outline,
                                               engine_->ftLibrary(),
                                               pixelMode,
                                               monoColorTable_,
                                               grayColorTable_);
      glyphScene_->addItem(currentGlyphBitmapItem_);
    }

    if (settingPanel_->showOutLinesChecked())
    {
      currentGlyphOutlineItem_ = new GlyphOutline(outlinePen_, outline);
      glyphScene_->addItem(currentGlyphOutlineItem_);
    }

    if (settingPanel_->showPointsChecked())
    {
      currentGlyphPointsItem_ = new GlyphPoints(onPen_, offPen_, outline);
      glyphScene_->addItem(currentGlyphPointsItem_);

      if (settingPanel_->showPointNumbersChecked())
      {
        currentGlyphPointNumbersItem_ = new GlyphPointNumbers(onPen_,
                                                             offPen_,
                                                             outline);
        glyphScene_->addItem(currentGlyphPointNumbersItem_);
      }
    }
  }

  glyphScene_->update();
}


// XXX distances are specified in pixels,
//     making the layout dependent on the output device resolution
void
MainGUI::createLayout()
{
  // left side
  fontFilenameLabel_ = new QLabel(this);

  infoLeftLayout_ = new QHBoxLayout;
  infoLeftLayout_->addWidget(fontFilenameLabel_);

  settingPanel_ = new SettingPanel(this, engine_);

  leftLayout_ = new QVBoxLayout;
  leftLayout_->addLayout(infoLeftLayout_);
  leftLayout_->addWidget(settingPanel_);

  // we don't want to expand the left side horizontally;
  // to change the policy we have to use a widget wrapper
  leftWidget_ = new QWidget(this);
  leftWidget_->setLayout(leftLayout_);

  QSizePolicy leftWidgetPolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  leftWidgetPolicy.setHorizontalStretch(0);
  leftWidgetPolicy.setVerticalPolicy(leftWidget_->sizePolicy().verticalPolicy());
  leftWidgetPolicy.setHeightForWidth(leftWidget_->sizePolicy().hasHeightForWidth());

  leftWidget_->setSizePolicy(leftWidgetPolicy);

  // right side
  glyphIndexLabel_ = new QLabel(this);
  glyphNameLabel_ = new QLabel(this);
  fontNameLabel_ = new QLabel(this);

  glyphScene_ = new QGraphicsScene(this);
  glyphScene_->addItem(new Grid(gridPen_, axisPen_));

  currentGlyphBitmapItem_ = NULL;
  currentGlyphOutlineItem_ = NULL;
  currentGlyphPointsItem_ = NULL;
  currentGlyphPointNumbersItem_ = NULL;

  glyphView_ = new QGraphicsViewx(this);
  glyphView_->setRenderHint(QPainter::Antialiasing, true);
  glyphView_->setDragMode(QGraphicsView::ScrollHandDrag);
  glyphView_->setOptimizationFlags(QGraphicsView::DontSavePainterState);
  glyphView_->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
  glyphView_->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  glyphView_->setScene(glyphScene_);

  // Don't use QGraphicsTextItem: We want this hint to be anchored at the
  // top-left corner.
  mouseUsageHint_ = new QLabel(tr("Scroll: Grid Up/Down\n"
    "Alt + Scroll: Grid Left/Right\n"
    "Ctrl + Scroll: Adjust Zoom (Relative to cursor)\n"
    "Shift + Scroll: Adjust Font Size"), glyphView_);
  mouseUsageHint_->setMargin(10);
  auto hintFont = font();
  hintFont.setPixelSize(24);
  mouseUsageHint_->setFont(hintFont);

  sizeLabel_ = new QLabel(tr("Size "), this);
  sizeLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  sizeDoubleSpinBox_ = new QDoubleSpinBox;
  sizeDoubleSpinBox_->setAlignment(Qt::AlignRight);
  sizeDoubleSpinBox_->setDecimals(1);
  sizeDoubleSpinBox_->setRange(1, 500);
  sizeLabel_->setBuddy(sizeDoubleSpinBox_);

  unitsComboBox_ = new QComboBox(this);
  unitsComboBox_->insertItem(Units_px, "px");
  unitsComboBox_->insertItem(Units_pt, "pt");

  dpiLabel_ = new QLabel(tr("DPI "), this);
  dpiLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  dpiSpinBox_ = new QSpinBox(this);
  dpiSpinBox_->setAlignment(Qt::AlignRight);
  dpiSpinBox_->setRange(10, 600);
  dpiLabel_->setBuddy(dpiSpinBox_);

  toStartButtonx_ = new QPushButtonx("|<", this);
  toM1000Buttonx_ = new QPushButtonx("-1000", this);
  toM100Buttonx_ = new QPushButtonx("-100", this);
  toM10Buttonx_ = new QPushButtonx("-10", this);
  toM1Buttonx_ = new QPushButtonx("-1", this);
  toP1Buttonx_ = new QPushButtonx("+1", this);
  toP10Buttonx_ = new QPushButtonx("+10", this);
  toP100Buttonx_ = new QPushButtonx("+100", this);
  toP1000Buttonx_ = new QPushButtonx("+1000", this);
  toEndButtonx_ = new QPushButtonx(">|", this);

  zoomLabel_ = new QLabel(tr("Zoom Factor"), this);
  zoomLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  zoomSpinBox_ = new QSpinBoxx(this);
  zoomSpinBox_->setAlignment(Qt::AlignRight);
  zoomSpinBox_->setRange(1, 1000 - 1000 % 64);
  zoomSpinBox_->setKeyboardTracking(false);
  zoomLabel_->setBuddy(zoomSpinBox_);

  centerGridButton_ = new QPushButton("Go Back to Grid Center", this);

  previousFontButton_ = new QPushButton(tr("Previous Font"), this);
  nextFontButton_ = new QPushButton(tr("Next Font"), this);
  previousFaceButton_ = new QPushButton(tr("Previous Face"), this);
  nextFaceButton_ = new QPushButton(tr("Next Face"), this);
  previousNamedInstanceButton_
    = new QPushButton(tr("Previous Named Instance"), this);
  nextNamedInstanceButton_ = new QPushButton(tr("Next Named Instance"), this);

  infoRightLayout = new QGridLayout;
  infoRightLayout->addWidget(glyphIndexLabel_, 0, 0);
  infoRightLayout->addWidget(glyphNameLabel_, 0, 1);
  infoRightLayout->addWidget(fontNameLabel_, 0, 2);

  navigationLayout_ = new QHBoxLayout;
  navigationLayout_->setSpacing(0);
  navigationLayout_->addStretch(1);
  navigationLayout_->addWidget(toStartButtonx_);
  navigationLayout_->addWidget(toM1000Buttonx_);
  navigationLayout_->addWidget(toM100Buttonx_);
  navigationLayout_->addWidget(toM10Buttonx_);
  navigationLayout_->addWidget(toM1Buttonx_);
  navigationLayout_->addWidget(toP1Buttonx_);
  navigationLayout_->addWidget(toP10Buttonx_);
  navigationLayout_->addWidget(toP100Buttonx_);
  navigationLayout_->addWidget(toP1000Buttonx_);
  navigationLayout_->addWidget(toEndButtonx_);
  navigationLayout_->addStretch(1);

  sizeLayout_ = new QHBoxLayout;
  sizeLayout_->addStretch(2);
  sizeLayout_->addWidget(sizeLabel_);
  sizeLayout_->addWidget(sizeDoubleSpinBox_);
  sizeLayout_->addWidget(unitsComboBox_);
  sizeLayout_->addStretch(1);
  sizeLayout_->addWidget(dpiLabel_);
  sizeLayout_->addWidget(dpiSpinBox_);
  sizeLayout_->addStretch(1);
  sizeLayout_->addWidget(zoomLabel_);
  sizeLayout_->addWidget(zoomSpinBox_);
  sizeLayout_->addStretch(1);
  sizeLayout_->addWidget(centerGridButton_);
  sizeLayout_->addStretch(2);

  fontLayout = new QGridLayout;
  fontLayout->setColumnStretch(0, 2);
  fontLayout->addWidget(nextFontButton_, 0, 1);
  fontLayout->addWidget(previousFontButton_, 1, 1);
  fontLayout->setColumnStretch(2, 1);
  fontLayout->addWidget(nextFaceButton_, 0, 3);
  fontLayout->addWidget(previousFaceButton_, 1, 3);
  fontLayout->setColumnStretch(4, 1);
  fontLayout->addWidget(nextNamedInstanceButton_, 0, 5);
  fontLayout->addWidget(previousNamedInstanceButton_, 1, 5);
  fontLayout->setColumnStretch(6, 2);

  rightLayout_ = new QVBoxLayout;
  rightLayout_->addLayout(infoRightLayout);
  rightLayout_->addWidget(glyphView_);
  rightLayout_->addLayout(navigationLayout_);
  rightLayout_->addSpacing(10); // XXX px
  rightLayout_->addLayout(sizeLayout_);
  rightLayout_->addSpacing(10); // XXX px
  rightLayout_->addLayout(fontLayout);

  // for symmetry with the left side use a widget also
  rightWidget_ = new QWidget(this);
  rightWidget_->setLayout(rightLayout_);

  // the whole thing
  ftinspectLayout_ = new QHBoxLayout;
  ftinspectLayout_->addWidget(leftWidget_);
  ftinspectLayout_->addWidget(rightWidget_);

  ftinspectWidget_ = new QWidget(this);
  ftinspectWidget_->setLayout(ftinspectLayout_);
  setCentralWidget(ftinspectWidget_);
  setWindowTitle("ftinspect");
}


void
MainGUI::createConnections()
{
  connect(settingPanel_, SIGNAL(fontReloadNeeded()),
          SLOT(showFont()));
  connect(settingPanel_, SIGNAL(repaintNeeded()),
          SLOT(drawGlyph()));
  connect(sizeDoubleSpinBox_, SIGNAL(valueChanged(double)),
          SLOT(drawGlyph()));
  connect(unitsComboBox_, SIGNAL(currentIndexChanged(int)),
          SLOT(checkUnits()));
  connect(dpiSpinBox_, SIGNAL(valueChanged(int)),
          SLOT(drawGlyph()));

  connect(zoomSpinBox_, SIGNAL(valueChanged(int)),
          SLOT(zoom()));
  connect(glyphView_, SIGNAL(shiftWheelEvent(QWheelEvent*)), 
          SLOT(wheelResize(QWheelEvent*)));
  connect(glyphView_, SIGNAL(ctrlWheelEvent(QWheelEvent*)), 
          SLOT(wheelZoom(QWheelEvent*)));

  connect(centerGridButton_, SIGNAL(clicked()),
          SLOT(backToCenter()));

  connect(previousFontButton_, SIGNAL(clicked()),
          SLOT(previousFont()));
  connect(nextFontButton_, SIGNAL(clicked()),
          SLOT(nextFont()));
  connect(previousFaceButton_, SIGNAL(clicked()),
          SLOT(previousFace()));
  connect(nextFaceButton_, SIGNAL(clicked()),
          SLOT(nextFace()));
  connect(previousNamedInstanceButton_, SIGNAL(clicked()),
          SLOT(previousNamedInstance()));
  connect(nextNamedInstanceButton_, SIGNAL(clicked()),
          SLOT(nextNamedInstance()));

  glyphNavigationMapper_ = new QSignalMapper;
  connect(glyphNavigationMapper_, SIGNAL(mapped(int)),
          SLOT(adjustGlyphIndex(int)));

  connect(toStartButtonx_, SIGNAL(clicked()),
          glyphNavigationMapper_, SLOT(map()));
  connect(toM1000Buttonx_, SIGNAL(clicked()),
          glyphNavigationMapper_, SLOT(map()));
  connect(toM100Buttonx_, SIGNAL(clicked()),
          glyphNavigationMapper_, SLOT(map()));
  connect(toM10Buttonx_, SIGNAL(clicked()),
          glyphNavigationMapper_, SLOT(map()));
  connect(toM1Buttonx_, SIGNAL(clicked()),
          glyphNavigationMapper_, SLOT(map()));
  connect(toP1Buttonx_, SIGNAL(clicked()),
          glyphNavigationMapper_, SLOT(map()));
  connect(toP10Buttonx_, SIGNAL(clicked()),
          glyphNavigationMapper_, SLOT(map()));
  connect(toP100Buttonx_, SIGNAL(clicked()),
          glyphNavigationMapper_, SLOT(map()));
  connect(toP1000Buttonx_, SIGNAL(clicked()),
          glyphNavigationMapper_, SLOT(map()));
  connect(toEndButtonx_, SIGNAL(clicked()),
          glyphNavigationMapper_, SLOT(map()));

  glyphNavigationMapper_->setMapping(toStartButtonx_, -0x10000);
  glyphNavigationMapper_->setMapping(toM1000Buttonx_, -1000);
  glyphNavigationMapper_->setMapping(toM100Buttonx_, -100);
  glyphNavigationMapper_->setMapping(toM10Buttonx_, -10);
  glyphNavigationMapper_->setMapping(toM1Buttonx_, -1);
  glyphNavigationMapper_->setMapping(toP1Buttonx_, 1);
  glyphNavigationMapper_->setMapping(toP10Buttonx_, 10);
  glyphNavigationMapper_->setMapping(toP100Buttonx_, 100);
  glyphNavigationMapper_->setMapping(toP1000Buttonx_, 1000);
  glyphNavigationMapper_->setMapping(toEndButtonx_, 0x10000);

  connect(&engine_->fontFileManager(), &FontFileManager::currentFileChanged,
          this, &MainGUI::watchCurrentFont);
}


void
MainGUI::createActions()
{
  loadFontsAct_ = new QAction(tr("&Load Fonts"), this);
  loadFontsAct_->setShortcuts(QKeySequence::Open);
  connect(loadFontsAct_, SIGNAL(triggered()), SLOT(loadFonts()));

  closeFontAct_ = new QAction(tr("&Close Font"), this);
  closeFontAct_->setShortcuts(QKeySequence::Close);
  connect(closeFontAct_, SIGNAL(triggered()), SLOT(closeFont()));

  exitAct_ = new QAction(tr("E&xit"), this);
  exitAct_->setShortcuts(QKeySequence::Quit);
  connect(exitAct_, SIGNAL(triggered()), SLOT(close()));

  aboutAct_ = new QAction(tr("&About"), this);
  connect(aboutAct_, SIGNAL(triggered()), SLOT(about()));

  aboutQtAct_ = new QAction(tr("About &Qt"), this);
  connect(aboutQtAct_, SIGNAL(triggered()), SLOT(aboutQt()));
}


void
MainGUI::createMenus()
{
  menuFile_ = menuBar()->addMenu(tr("&File"));
  menuFile_->addAction(loadFontsAct_);
  menuFile_->addAction(closeFontAct_);
  menuFile_->addAction(exitAct_);

  menuHelp_ = menuBar()->addMenu(tr("&Help"));
  menuHelp_->addAction(aboutAct_);
  menuHelp_->addAction(aboutQtAct_);
}


void
MainGUI::createStatusBar()
{
  statusBar()->showMessage("");
}


void
MainGUI::setDefaults()
{
  // the next four values always non-negative
  currentFontIndex_ = 0;
  currentFaceIndex_ = 0;
  currentNamedInstanceIndex_ = 0;
  currentGlyphIndex_ = 0;

  sizeDoubleSpinBox_->setValue(20);
  dpiSpinBox_->setValue(96);
  zoomSpinBox_->setValue(20);

  // todo run check for settingpanel
  checkUnits();
  checkCurrentFontIndex();
  checkCurrentFaceIndex();
  checkCurrentNamedInstanceIndex();
  adjustGlyphIndex(0);
  zoom();
}


void
MainGUI::readSettings()
{
  QSettings settings;
//  QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
//  QSize size = settings.value("size", QSize(400, 400)).toSize();
//  resize(size);
//  move(pos);
}


void
MainGUI::writeSettings()
{
  QSettings settings;
//  settings.setValue("pos", pos());
//  settings.setValue("size", size());
}


// end of maingui.cpp
