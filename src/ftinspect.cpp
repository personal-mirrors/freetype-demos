// ftinspect.cpp

// Copyright (C) 2016 by Werner Lemberg.

#include "ftinspect.h"


#define VERSION "X.Y.Z"


// The face requester is a function provided by the client application to
// the cache manager to translate an `abstract' face ID into a real
// `FT_Face' object.
//
// Here, the face IDs are simply pointers to `Font' objects.

static FT_Error
faceRequester(FTC_FaceID faceID,
              FT_Library library,
              FT_Pointer /* requestData */,
              FT_Face* faceP)
{
  Font* font = static_cast<Font*>(faceID);

  return FT_New_Face(library,
                     font->filePathname,
                     font->faceIndex,
                     faceP);
}


Engine::Engine()
{
  FT_Error error;

  error = FT_Init_FreeType(&library);
  if (error)
  {
    // XXX error handling
  }

  error = FTC_Manager_New(library, 0, 0, 0,
                          faceRequester, 0, &cacheManager);
  if (error)
  {
    // XXX error handling
  }

  error = FTC_SBitCache_New(cacheManager, &sbitsCache);
  if (error)
  {
    // XXX error handling
  }

  error = FTC_ImageCache_New(cacheManager, &imageCache);
  if (error)
  {
    // XXX error handling
  }
}


Engine::~Engine()
{
  FTC_Manager_Done(cacheManager);
  FT_Done_FreeType(library);
}


void
Engine::update(const MainGUI& gui)
{
  dpi = gui.dpiSpinBox->value();
  zoom = gui.zoomSpinBox->value();

  if (gui.unitsComboBox->currentIndex() == MainGUI::Units_px)
  {
    pointSize = gui.sizeDoubleSpinBox->value();
    pixelSize = pointSize * dpi / 72.0;
  }
  else
  {
    pixelSize = gui.sizeDoubleSpinBox->value();
    pointSize = pixelSize * 72.0 / dpi;
  }

  doHorizontalHinting = gui.horizontalHintingCheckBox->isChecked();
  doVerticalHinting = gui.verticalHintingCheckBox->isChecked();
  doBlueZoneHinting = gui.blueZoneHintingCheckBox->isChecked();
  showSegments = gui.segmentDrawingCheckBox->isChecked();
  doWarping = gui.warpingCheckBox->isChecked();

  showBitmap = gui.showBitmapCheckBox->isChecked();
  showPoints = gui.showPointsCheckBox->isChecked();
  if (showPoints)
    showPointIndices = gui.showPointIndicesCheckBox->isChecked();
  else
    showPointIndices = false;
  showOutlines = gui.showOutlinesCheckBox->isChecked();

  gamma = gui.gammaSlider->value();
}


MainGUI::MainGUI()
{
  createLayout();
  createConnections();
  createActions();
  createMenus();
  createStatusBar();

  setDefaults();
  readSettings();

  setUnifiedTitleAndToolBarOnMac(true);
}


MainGUI::~MainGUI()
{
}


void
MainGUI::update(const Engine* e)
{
  engine = e;
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
       " Copyright %2 2016<br>"
       " by Werner Lemberg <tt>&lt;wl@gnu.org&gt;</tt></p>"
       ""
       "<p><b>ftinspect</b> shows how a font gets rendered"
       " by FreeType, allowing control over virtually"
       " all rendering parameters.</p>"
       ""
       "<p>License:"
       " <a href='http://git.savannah.gnu.org/cgit/freetype/freetype2.git/tree/docs/FTL.TXT'>FreeType"
       " License (FTL)</a> or"
       " <a href='http://git.savannah.gnu.org/cgit/freetype/freetype2.git/tree/docs/GPLv2.TXT'>GNU"
       " GPLv2</a></p>")
       .arg(VERSION)
       .arg(QChar(0xA9)));
}


void
MainGUI::loadFonts()
{
  int oldSize = fontFileNames.size();

  QStringList files = QFileDialog::getOpenFileNames(
                        this,
                        tr("Load one or more fonts"),
                        QDir::homePath(),
                        "",
                        NULL,
                        QFileDialog::ReadOnly);
  fontFileNames += files;

  // if we have new fonts, set the current index to the first new one
  if (!fontFileNames.isEmpty()
      && oldSize < fontFileNames.size())
    currentFontFileIndex = oldSize;

  checkCurrentFontFileIndex();

  // XXX trigger redisplay
}


void
MainGUI::closeFont()
{
  if (currentFontFileIndex >= 0)
    fontFileNames.removeAt(currentFontFileIndex);
  if (currentFontFileIndex >= fontFileNames.size())
    currentFontFileIndex--;

  checkCurrentFontFileIndex();

  // XXX trigger redisplay
}


void
MainGUI::checkHintingMode()
{
  int index = hintingModeComboBoxx->currentIndex();
  int AAcurrIndex = antiAliasingComboBoxx->currentIndex();

  if (index == HintingMode_AutoHinting)
  {
    horizontalHintingCheckBox->setEnabled(true);
    verticalHintingCheckBox->setEnabled(true);
    blueZoneHintingCheckBox->setEnabled(true);
    segmentDrawingCheckBox->setEnabled(true);
    warpingCheckBox->setEnabled(true);

    antiAliasingComboBoxx->setItemEnabled(AntiAliasing_Slight, true);
  }
  else
  {
    horizontalHintingCheckBox->setEnabled(false);
    verticalHintingCheckBox->setEnabled(false);
    blueZoneHintingCheckBox->setEnabled(false);
    segmentDrawingCheckBox->setEnabled(false);
    warpingCheckBox->setEnabled(false);

    antiAliasingComboBoxx->setItemEnabled(AntiAliasing_Slight, false);

    if (AAcurrIndex == AntiAliasing_Slight)
      antiAliasingComboBoxx->setCurrentIndex(AntiAliasing_Normal);
  }
}


void
MainGUI::checkAntiAliasing()
{
  int index = antiAliasingComboBoxx->currentIndex();

  if (index == AntiAliasing_None
      || index == AntiAliasing_Normal
      || index == AntiAliasing_Slight)
  {
    lcdFilterLabel->setEnabled(false);
    lcdFilterComboBox->setEnabled(false);
  }
  else
  {
    lcdFilterLabel->setEnabled(true);
    lcdFilterComboBox->setEnabled(true);
  }
}


void
MainGUI::checkShowPoints()
{
  if (showPointsCheckBox->isChecked())
    showPointIndicesCheckBox->setEnabled(true);
  else
    showPointIndicesCheckBox->setEnabled(false);
}


void
MainGUI::checkUnits()
{
  int index = unitsComboBox->currentIndex();

  if (index == Units_px)
  {
    dpiLabel->setEnabled(false);
    dpiSpinBox->setEnabled(false);
  }
  else
  {
    dpiLabel->setEnabled(true);
    dpiSpinBox->setEnabled(true);
  }
}


void
MainGUI::checkCurrentFontFileIndex()
{
  if (fontFileNames.size() < 2)
  {
    previousFontButton->setEnabled(false);
    nextFontButton->setEnabled(false);
  }
  else if (currentFontFileIndex == 0)
  {
    previousFontButton->setEnabled(false);
    nextFontButton->setEnabled(true);
  }
  else if (currentFontFileIndex == fontFileNames.size() - 1)
  {
    previousFontButton->setEnabled(true);
    nextFontButton->setEnabled(false);
  }
  else
  {
    previousFontButton->setEnabled(true);
    nextFontButton->setEnabled(true);
  }
}


void
MainGUI::checkCurrentFaceIndex()
{
  if (numFaces < 2)
  {
    previousFaceButton->setEnabled(false);
    nextFaceButton->setEnabled(false);
  }
  else if (currentFaceIndex == 0)
  {
    previousFaceButton->setEnabled(false);
    nextFaceButton->setEnabled(true);
  }
  else if (currentFaceIndex == numFaces - 1)
  {
    previousFaceButton->setEnabled(true);
    nextFaceButton->setEnabled(false);
  }
  else
  {
    previousFaceButton->setEnabled(true);
    nextFaceButton->setEnabled(true);
  }
}


void
MainGUI::previousFont()
{
  if (currentFontFileIndex > 0)
  {
    currentFontFileIndex--;
    checkCurrentFontFileIndex();
  }
}


void
MainGUI::nextFont()
{
  if (currentFontFileIndex < fontFileNames.size() - 1)
  {
    currentFontFileIndex++;
    checkCurrentFontFileIndex();
  }
}


void
MainGUI::previousFace()
{
  if (currentFaceIndex > 0)
  {
    currentFaceIndex--;
    checkCurrentFaceIndex();
  }
}


void
MainGUI::nextFace()
{
  if (currentFaceIndex < numFaces - 1)
  {
    currentFaceIndex++;
    checkCurrentFaceIndex();
  }
}


// XXX distances are specified in pixels,
//     making the layout dependent on the output device resolution
void
MainGUI::createLayout()
{
  // left side
  hintingModeLabel = new QLabel(tr("Hinting Mode"));
  hintingModeLabel->setAlignment(Qt::AlignRight);
  hintingModeComboBoxx = new QComboBoxx;
  hintingModeComboBoxx->insertItem(HintingMode_TrueType_v35,
                                   tr("TrueType v35"));
  hintingModeComboBoxx->insertItem(HintingMode_TrueType_v38,
                                   tr("TrueType v38"));
  hintingModeComboBoxx->insertItem(HintingMode_TrueType_v40,
                                   tr("TrueType v40"));
  hintingModeComboBoxx->insertItem(HintingMode_CFF_FreeType,
                                   tr("CFF (FreeType)"));
  hintingModeComboBoxx->insertItem(HintingMode_CFF_Adobe,
                                   tr("CFF (Adobe)"));
  hintingModeComboBoxx->insertItem(HintingMode_AutoHinting,
                                   tr("Auto-Hinting"));
  hintingModeLabel->setBuddy(hintingModeComboBoxx);

  horizontalHintingCheckBox = new QCheckBox(tr("Horizontal Hinting"));
  verticalHintingCheckBox = new QCheckBox(tr("Vertical Hinting"));
  blueZoneHintingCheckBox = new QCheckBox(tr("Blue-Zone Hinting"));
  segmentDrawingCheckBox = new QCheckBox(tr("Segment Drawing"));
  warpingCheckBox = new QCheckBox(tr("Warping"));

  antiAliasingLabel = new QLabel(tr("Anti-Aliasing"));
  antiAliasingLabel->setAlignment(Qt::AlignRight);
  antiAliasingComboBoxx = new QComboBoxx;
  antiAliasingComboBoxx->insertItem(AntiAliasing_None,
                                    tr("None"));
  antiAliasingComboBoxx->insertItem(AntiAliasing_Normal,
                                    tr("Normal"));
  antiAliasingComboBoxx->insertItem(AntiAliasing_Slight,
                                    tr("Slight"));
  antiAliasingComboBoxx->insertItem(AntiAliasing_LCD,
                                    tr("LCD (RGB)"));
  antiAliasingComboBoxx->insertItem(AntiAliasing_LCD_BGR,
                                    tr("LCD (BGR)"));
  antiAliasingComboBoxx->insertItem(AntiAliasing_LCD_Vertical,
                                    tr("LCD (vert. RGB)"));
  antiAliasingComboBoxx->insertItem(AntiAliasing_LCD_Vertical_BGR,
                                    tr("LCD (vert. BGR)"));
  antiAliasingLabel->setBuddy(antiAliasingComboBoxx);

  lcdFilterLabel = new QLabel(tr("LCD Filter"));
  lcdFilterLabel->setAlignment(Qt::AlignRight);
  lcdFilterComboBox = new QComboBox;
  lcdFilterComboBox->insertItem(LCDFilter_Default, tr("Default"));
  lcdFilterComboBox->insertItem(LCDFilter_Light, tr("Light"));
  lcdFilterComboBox->insertItem(LCDFilter_None, tr("None"));
  lcdFilterComboBox->insertItem(LCDFilter_Legacy, tr("Legacy"));
  lcdFilterLabel->setBuddy(lcdFilterComboBox);

  int width;
  // make all labels have the same width
  width = hintingModeLabel->minimumSizeHint().width();
  width = qMax(antiAliasingLabel->minimumSizeHint().width(), width);
  width = qMax(lcdFilterLabel->minimumSizeHint().width(), width);
  hintingModeLabel->setMinimumWidth(width);
  antiAliasingLabel->setMinimumWidth(width);
  lcdFilterLabel->setMinimumWidth(width);

  // ensure that all items in combo boxes fit completely;
  // also make all combo boxes have the same width
  width = hintingModeComboBoxx->minimumSizeHint().width();
  width = qMax(antiAliasingComboBoxx->minimumSizeHint().width(), width);
  width = qMax(lcdFilterComboBox->minimumSizeHint().width(), width);
  hintingModeComboBoxx->setMinimumWidth(width);
  antiAliasingComboBoxx->setMinimumWidth(width);
  lcdFilterComboBox->setMinimumWidth(width);

  gammaLabel = new QLabel(tr("Gamma"));
  gammaLabel->setAlignment(Qt::AlignRight);
  gammaSlider = new QSlider(Qt::Horizontal);
  gammaSlider->setRange(0, 30); // in 1/10th
  gammaSlider->setTickPosition(QSlider::TicksBelow);
  gammaLabel->setBuddy(gammaSlider);

  showBitmapCheckBox = new QCheckBox(tr("Show Bitmap"));
  showPointsCheckBox = new QCheckBox(tr("Show Points"));
  showPointIndicesCheckBox = new QCheckBox(tr("Show Point Indices"));
  showOutlinesCheckBox = new QCheckBox(tr("Show Outlines"));

  watchButton = new QPushButton(tr("Watch"));

  hintingModeLayout = new QHBoxLayout;
  hintingModeLayout->addWidget(hintingModeLabel);
  hintingModeLayout->addWidget(hintingModeComboBoxx);

  antiAliasingLayout = new QHBoxLayout;
  antiAliasingLayout->addWidget(antiAliasingLabel);
  antiAliasingLayout->addWidget(antiAliasingComboBoxx);

  lcdFilterLayout = new QHBoxLayout;
  lcdFilterLayout->addWidget(lcdFilterLabel);
  lcdFilterLayout->addWidget(lcdFilterComboBox);

  gammaLayout = new QHBoxLayout;
  gammaLayout->addWidget(gammaLabel);
  gammaLayout->addWidget(gammaSlider);

  generalTabLayout = new QVBoxLayout;
  generalTabLayout->addLayout(hintingModeLayout);
  generalTabLayout->addWidget(horizontalHintingCheckBox);
  generalTabLayout->addWidget(verticalHintingCheckBox);
  generalTabLayout->addWidget(blueZoneHintingCheckBox);
  generalTabLayout->addWidget(segmentDrawingCheckBox);
  generalTabLayout->addWidget(warpingCheckBox);
  generalTabLayout->addSpacing(20); // XXX px
  generalTabLayout->addStretch(1);
  generalTabLayout->addLayout(antiAliasingLayout);
  generalTabLayout->addLayout(lcdFilterLayout);
  generalTabLayout->addSpacing(20); // XXX px
  generalTabLayout->addStretch(1);
  generalTabLayout->addLayout(gammaLayout);
  generalTabLayout->addSpacing(20); // XXX px
  generalTabLayout->addStretch(1);
  generalTabLayout->addWidget(showBitmapCheckBox);
  generalTabLayout->addWidget(showPointsCheckBox);
  generalTabLayout->addWidget(showPointIndicesCheckBox);
  generalTabLayout->addWidget(showOutlinesCheckBox);

  generalTabWidget = new QWidget;
  generalTabWidget->setLayout(generalTabLayout);

  mmgxTabWidget = new QWidget;

  watchLayout = new QHBoxLayout;
  watchLayout->addStretch(1);
  watchLayout->addWidget(watchButton);
  watchLayout->addStretch(1);

  tabWidget = new QTabWidget;
  tabWidget->addTab(generalTabWidget, tr("General"));
  tabWidget->addTab(mmgxTabWidget, tr("MM/GX"));

  leftLayout = new QVBoxLayout;
  leftLayout->addWidget(tabWidget);
  leftLayout->addSpacing(10); // XXX px
  leftLayout->addLayout(watchLayout);

  // we don't want to expand the left side horizontally;
  // to change the policy we have to use a widget wrapper
  leftWidget = new QWidget;
  leftWidget->setLayout(leftLayout);

  QSizePolicy leftWidgetPolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  leftWidgetPolicy.setHorizontalStretch(0);
  leftWidgetPolicy.setVerticalPolicy(leftWidget->sizePolicy().verticalPolicy());
  leftWidgetPolicy.setHeightForWidth(leftWidget->sizePolicy().hasHeightForWidth());

  leftWidget->setSizePolicy(leftWidgetPolicy);

  // right side
  glyphView = new QGraphicsView;

  sizeLabel = new QLabel(tr("Size "));
  sizeLabel->setAlignment(Qt::AlignRight);
  sizeDoubleSpinBox = new QDoubleSpinBox;
  sizeDoubleSpinBox->setAlignment(Qt::AlignRight);
  sizeDoubleSpinBox->setDecimals(1);
  sizeDoubleSpinBox->setRange(1, 500);
  sizeDoubleSpinBox->setSingleStep(0.5);
  sizeDoubleSpinBox->setValue(20); // XXX default
  sizeLabel->setBuddy(sizeDoubleSpinBox);

  unitsComboBox = new QComboBox;
  unitsComboBox->insertItem(Units_px, "px");
  unitsComboBox->insertItem(Units_pt, "pt");

  dpiLabel = new QLabel(tr("DPI "));
  dpiLabel->setAlignment(Qt::AlignRight);
  dpiSpinBox = new QSpinBox;
  dpiSpinBox->setAlignment(Qt::AlignRight);
  dpiSpinBox->setRange(10, 600);
  dpiSpinBox->setValue(96); // XXX default
  dpiLabel->setBuddy(dpiSpinBox);

  toStartButtonx = new QPushButtonx("|<");
  toM1000Buttonx = new QPushButtonx("-1000");
  toM100Buttonx = new QPushButtonx("-100");
  toM10Buttonx = new QPushButtonx("-10");
  toM1Buttonx = new QPushButtonx("-1");
  toP1Buttonx = new QPushButtonx("+1");
  toP10Buttonx = new QPushButtonx("+10");
  toP100Buttonx = new QPushButtonx("+100");
  toP1000Buttonx = new QPushButtonx("+1000");
  toEndButtonx = new QPushButtonx(">|");

  zoomLabel = new QLabel(tr("Zoom "));
  zoomLabel->setAlignment(Qt::AlignRight);
  zoomSpinBox = new QSpinBox;
  zoomSpinBox->setAlignment(Qt::AlignRight);
  zoomSpinBox->setRange(1, 10000);
  zoomSpinBox->setSuffix("%");
  zoomSpinBox->setSingleStep(10);
  zoomSpinBox->setValue(100); // XXX default
  zoomLabel->setBuddy(zoomSpinBox);

  previousFontButton = new QPushButton(tr("Previous Font"));
  nextFontButton = new QPushButton(tr("Next Font"));
  previousFaceButton = new QPushButton(tr("Previous Face"));
  nextFaceButton = new QPushButton(tr("Next Face"));

  navigationLayout = new QHBoxLayout;
  navigationLayout->setSpacing(0);
  navigationLayout->addStretch(1);
  navigationLayout->addWidget(toStartButtonx);
  navigationLayout->addWidget(toM1000Buttonx);
  navigationLayout->addWidget(toM100Buttonx);
  navigationLayout->addWidget(toM10Buttonx);
  navigationLayout->addWidget(toM1Buttonx);
  navigationLayout->addWidget(toP1Buttonx);
  navigationLayout->addWidget(toP10Buttonx);
  navigationLayout->addWidget(toP100Buttonx);
  navigationLayout->addWidget(toP1000Buttonx);
  navigationLayout->addWidget(toEndButtonx);
  navigationLayout->addStretch(1);

  sizeLayout = new QHBoxLayout;
  sizeLayout->addStretch(2);
  sizeLayout->addWidget(sizeLabel);
  sizeLayout->addWidget(sizeDoubleSpinBox);
  sizeLayout->addWidget(unitsComboBox);
  sizeLayout->addStretch(1);
  sizeLayout->addWidget(dpiLabel);
  sizeLayout->addWidget(dpiSpinBox);
  sizeLayout->addStretch(1);
  sizeLayout->addWidget(zoomLabel);
  sizeLayout->addWidget(zoomSpinBox);
  sizeLayout->addStretch(2);

  fontLayout = new QHBoxLayout;
  fontLayout->addStretch(2);
  fontLayout->addWidget(previousFontButton);
  fontLayout->addStretch(1);
  fontLayout->addWidget(nextFontButton);
  fontLayout->addStretch(1);
  fontLayout->addWidget(previousFaceButton);
  fontLayout->addStretch(1);
  fontLayout->addWidget(nextFaceButton);
  fontLayout->addStretch(2);

  rightLayout = new QVBoxLayout;
  rightLayout->addWidget(glyphView);
  rightLayout->addLayout(navigationLayout);
  rightLayout->addSpacing(10); // XXX px
  rightLayout->addLayout(sizeLayout);
  rightLayout->addSpacing(10); // XXX px
  rightLayout->addLayout(fontLayout);

  // for symmetry with the left side use a widget also
  rightWidget = new QWidget;
  rightWidget->setLayout(rightLayout);

  // the whole thing
  ftinspectLayout = new QHBoxLayout;
  ftinspectLayout->addWidget(leftWidget);
  ftinspectLayout->addWidget(rightWidget);

  ftinspectWidget = new QWidget;
  ftinspectWidget->setLayout(ftinspectLayout);
  setCentralWidget(ftinspectWidget);
  setWindowTitle("ftinspect");
}


void
MainGUI::createConnections()
{
  connect(hintingModeComboBoxx, SIGNAL(currentIndexChanged(int)), this,
          SLOT(checkHintingMode()));
  connect(antiAliasingComboBoxx, SIGNAL(currentIndexChanged(int)), this,
          SLOT(checkAntiAliasing()));

  connect(showPointsCheckBox, SIGNAL(clicked()), this,
          SLOT(checkShowPoints()));

  connect(unitsComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(checkUnits()));

  connect(previousFontButton, SIGNAL(clicked()), this,
          SLOT(previousFont()));
  connect(nextFontButton, SIGNAL(clicked()), this,
          SLOT(nextFont()));
  connect(previousFaceButton, SIGNAL(clicked()), this,
          SLOT(previousFace()));
  connect(nextFaceButton, SIGNAL(clicked()), this,
          SLOT(nextFace()));
}


void
MainGUI::createActions()
{
  loadFontsAct = new QAction(tr("&Load Fonts"), this);
  loadFontsAct->setShortcuts(QKeySequence::Open);
  connect(loadFontsAct, SIGNAL(triggered()), this, SLOT(loadFonts()));

  closeFontAct = new QAction(tr("&Close Font"), this);
  closeFontAct->setShortcuts(QKeySequence::Close);
  connect(closeFontAct, SIGNAL(triggered()), this, SLOT(closeFont()));

  exitAct = new QAction(tr("E&xit"), this);
  exitAct->setShortcuts(QKeySequence::Quit);
  connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

  aboutAct = new QAction(tr("&About"), this);
  connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

  aboutQtAct = new QAction(tr("About &Qt"), this);
  connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}


void
MainGUI::createMenus()
{
  menuFile = menuBar()->addMenu(tr("&File"));
  menuFile->addAction(loadFontsAct);
  menuFile->addAction(closeFontAct);
  menuFile->addAction(exitAct);

  menuHelp = menuBar()->addMenu(tr("&Help"));
  menuHelp->addAction(aboutAct);
  menuHelp->addAction(aboutQtAct);
}


void
MainGUI::createStatusBar()
{
  statusBar()->showMessage("");
}


void
MainGUI::clearStatusBar()
{
  statusBar()->clearMessage();
  statusBar()->setStyleSheet("");
}


void
MainGUI::setDefaults()
{
  currentFontFileIndex = -1;

  // XXX only dummy values right now

  numFaces = 0;
  currentFaceIndex = -1;

  hintingModeComboBoxx->setCurrentIndex(HintingMode_TrueType_v35);
  antiAliasingComboBoxx->setCurrentIndex(AntiAliasing_LCD);
  lcdFilterComboBox->setCurrentIndex(LCDFilter_Light);

  horizontalHintingCheckBox->setChecked(true);
  verticalHintingCheckBox->setChecked(true);
  blueZoneHintingCheckBox->setChecked(true);

  showBitmapCheckBox->setChecked(true);
  showOutlinesCheckBox->setChecked(true);

  checkHintingMode();
  checkAntiAliasing();
  checkShowPoints();
  checkUnits();
  checkCurrentFontFileIndex();
  checkCurrentFaceIndex();
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


void
QComboBoxx::setItemEnabled(int index,
                           bool enable)
{
  const QStandardItemModel* itemModel =
    qobject_cast<const QStandardItemModel*>(model());
  QStandardItem* item = itemModel->item(index);

  if (enable)
  {
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setData(QVariant(),
                  Qt::TextColorRole);
  }
  else
  {
    item->setFlags(item->flags()
                   & ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled));
    // clear item data in order to use default color;
    // this visually greys out the item
    item->setData(palette().color(QPalette::Disabled, QPalette::Text),
                  Qt::TextColorRole);
  }
}


// code derived from Qt 4.8.7, function `QPushButton::sizeHint',
// file `src/gui/widgets/qpushbutton.cpp'

QPushButtonx::QPushButtonx(const QString &text,
                           QWidget *parent)
: QPushButton(text, parent)
{
  QStyleOptionButton opt;
  opt.initFrom(this);
  QString s(this->text());
  QFontMetrics fm = fontMetrics();
  QSize sz = fm.size(Qt::TextShowMnemonic, s);
  opt.rect.setSize(sz);

  sz = style()->sizeFromContents(QStyle::CT_PushButton,
                                 &opt,
                                 sz,
                                 this);
  setFixedWidth(sz.width());
}


int
main(int argc,
     char** argv)
{
  QApplication app(argc, argv);
  app.setApplicationName("ftinspect");
  app.setApplicationVersion(VERSION);
  app.setOrganizationName("FreeType");
  app.setOrganizationDomain("freetype.org");

  Engine engine;
  MainGUI gui;

  engine.update(gui);
  gui.update(&engine);

  gui.show();

  return app.exec();
}


// end of ftinspect.cpp
