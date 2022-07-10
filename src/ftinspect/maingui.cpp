// maingui.cpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#include "maingui.hpp"
#include "rendering/grid.hpp"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QScrollBar>
#include <QMimeData>

#include <freetype/ftdriver.h>

#include "panels/continuous.hpp"


MainGUI::MainGUI(Engine* engine)
: engine_(engine)
{
  createLayout();
  createConnections();
  createActions();
  createMenus();
  createStatusBar();
  setupDragDrop();

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
MainGUI::dragEnterEvent(QDragEnterEvent* event)
{
  if (event->mimeData()->hasUrls())
    event->acceptProposedAction();
}


void
MainGUI::dropEvent(QDropEvent* event)
{
  auto mime = event->mimeData();
  if (!mime->hasUrls())
    return;

  QStringList fileNames;
  for (auto& url : mime->urls())
  {
    if (!url.isLocalFile())
      continue;
    fileNames.append(url.toLocalFile());
  }

  if (fileNames.empty())
    return;

  event->acceptProposedAction();
  openFonts(fileNames);
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
  QStringList files = QFileDialog::getOpenFileNames(
                        this,
                        tr("Load one or more fonts"),
                        QDir::homePath(),
                        "",
                        NULL,
                        QFileDialog::ReadOnly);
  openFonts(files);
}


void
MainGUI::openFonts(QStringList const& fileNames)
{
  int oldSize = engine_->numberOfOpenedFonts();
  engine_->openFonts(fileNames);

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
  reloadCurrentTabFont();
}


void
MainGUI::repaintCurrentTab()
{
  syncSettings();
  tabs_[tabWidget_->currentIndex()]->repaintGlyph();
}


void
MainGUI::reloadCurrentTabFont()
{
  tabs_[tabWidget_->currentIndex()]->reloadFont();
}


void
MainGUI::syncSettings()
{
  settingPanel_->syncSettings();
  tabs_[tabWidget_->currentIndex()]->syncSettings();
}


void
MainGUI::clearStatusBar()
{
  statusBar()->clearMessage();
  statusBar()->setStyleSheet("");
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
  fontNameLabel_ = new QLabel(this);

  singularTab_ = new SingularTab(this, engine_);
  continuousTab_ = new ContinuousTab(this, engine_);

  tabWidget_ = new QTabWidget(this);

  // Note those two list must be in sync
  tabs_.append(singularTab_);
  tabWidget_->addTab(singularTab_, tr("Singular Grid View"));
  tabs_.append(continuousTab_);
  tabWidget_->addTab(continuousTab_, tr("Continuous View"));

  previousFontButton_ = new QPushButton(tr("Previous Font"), this);
  nextFontButton_ = new QPushButton(tr("Next Font"), this);
  previousFaceButton_ = new QPushButton(tr("Previous Face"), this);
  nextFaceButton_ = new QPushButton(tr("Next Face"), this);
  previousNamedInstanceButton_
    = new QPushButton(tr("Previous Named Instance"), this);
  nextNamedInstanceButton_ = new QPushButton(tr("Next Named Instance"), this);

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
  rightLayout_->addWidget(fontNameLabel_);
  rightLayout_->addWidget(tabWidget_);
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
  connect(settingPanel_, &SettingPanel::fontReloadNeeded,
          this, &MainGUI::showFont);
  connect(settingPanel_, &SettingPanel::repaintNeeded,
          this, &MainGUI::repaintCurrentTab);

  connect(tabWidget_, &QTabWidget::currentChanged,
          this, &MainGUI::reloadCurrentTabFont);

  connect(previousFontButton_, &QPushButton::clicked,
          this, &MainGUI::previousFont);
  connect(nextFontButton_, &QPushButton::clicked,
          this, &MainGUI::nextFont);
  connect(previousFaceButton_, &QPushButton::clicked,
          this, &MainGUI::previousFace);
  connect(nextFaceButton_, &QPushButton::clicked,
          this, &MainGUI::nextFace);
  connect(previousNamedInstanceButton_, &QPushButton::clicked,
          this, &MainGUI::previousNamedInstance);
  connect(nextNamedInstanceButton_, &QPushButton::clicked,
          this, &MainGUI::nextNamedInstance);

  connect(&engine_->fontFileManager(), &FontFileManager::currentFileChanged,
          this, &MainGUI::watchCurrentFont);
}


void
MainGUI::createActions()
{
  loadFontsAct_ = new QAction(tr("&Load Fonts"), this);
  loadFontsAct_->setShortcuts(QKeySequence::Open);
  connect(loadFontsAct_, &QAction::triggered, this, &MainGUI::loadFonts);

  closeFontAct_ = new QAction(tr("&Close Font"), this);
  closeFontAct_->setShortcuts(QKeySequence::Close);
  connect(closeFontAct_, &QAction::triggered, this, &MainGUI::closeFont);

  exitAct_ = new QAction(tr("E&xit"), this);
  exitAct_->setShortcuts(QKeySequence::Quit);
  connect(exitAct_, &QAction::triggered, this, &MainGUI::close);

  aboutAct_ = new QAction(tr("&About"), this);
  connect(aboutAct_, &QAction::triggered, this, &MainGUI::about);

  aboutQtAct_ = new QAction(tr("About &Qt"), this);
  connect(aboutQtAct_, &QAction::triggered, this, &MainGUI::aboutQt);
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
MainGUI::setupDragDrop()
{
  setAcceptDrops(true);
}


void
MainGUI::setDefaults()
{
  // the next four values always non-negative
  currentFontIndex_ = 0;
  currentFaceIndex_ = 0;
  currentNamedInstanceIndex_ = 0;

  for (auto tab : tabs_)
    tab->setDefaults();
  checkCurrentFontIndex();
  checkCurrentFaceIndex();
  checkCurrentNamedInstanceIndex();
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
