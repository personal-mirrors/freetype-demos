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

  tripletSelector_->repopulateFonts();
}


void
MainGUI::onTripletChanged()
{
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
  syncSettings();
  tabs_[tabWidget_->currentIndex()]->reloadFont();
}


void
MainGUI::syncSettings()
{
  settingPanel_->syncSettings();
}


// XXX distances are specified in pixels,
//     making the layout dependent on the output device resolution
void
MainGUI::createLayout()
{
  // left side
  settingPanel_ = new SettingPanel(this, engine_);

  leftLayout_ = new QVBoxLayout; // The only point is to set a margin->remove?
  leftLayout_->addWidget(settingPanel_);
  leftLayout_->setContentsMargins(32, 32, 8, 16);

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
  singularTab_ = new SingularTab(this, engine_);
  continuousTab_ = new ContinuousTab(this, engine_);

  tabWidget_ = new QTabWidget(this);

  // Note those two list must be in sync
  tabs_.append(singularTab_);
  tabWidget_->addTab(singularTab_, tr("Singular Grid View"));
  tabs_.append(continuousTab_);
  tabWidget_->addTab(continuousTab_, tr("Continuous View"));
  
  tripletSelector_ = new TripletSelector(this, engine_);

  rightLayout_ = new QVBoxLayout;
  //rightLayout_->addWidget(fontNameLabel_);
  rightLayout_->addWidget(tabWidget_); // same for `leftLayout_`: Remove?
  rightLayout_->setContentsMargins(8, 32, 32, 16);

  // for symmetry with the left side use a widget also
  rightWidget_ = new QWidget(this);
  rightWidget_->setLayout(rightLayout_);

  // the whole thing
  mainPartLayout_ = new QHBoxLayout;
  mainPartLayout_->addWidget(leftWidget_);
  mainPartLayout_->addWidget(rightWidget_);

  ftinspectLayout_ = new QVBoxLayout;
  ftinspectLayout_->setSpacing(0);
  ftinspectLayout_->addLayout(mainPartLayout_);
  ftinspectLayout_->addWidget(tripletSelector_);
  ftinspectLayout_->setContentsMargins(0, 0, 0, 0);

  ftinspectWidget_ = new QWidget(this);
  ftinspectWidget_->setLayout(ftinspectLayout_);

  statusBar()->hide(); // remove the extra space
  setCentralWidget(ftinspectWidget_);
  setWindowTitle("ftinspect");
}


void
MainGUI::createConnections()
{
  connect(settingPanel_, &SettingPanel::fontReloadNeeded,
          this, &MainGUI::reloadCurrentTabFont);
  connect(settingPanel_, &SettingPanel::repaintNeeded,
          this, &MainGUI::repaintCurrentTab);

  connect(tabWidget_, &QTabWidget::currentChanged,
          this, &MainGUI::reloadCurrentTabFont);

  connect(tripletSelector_, &TripletSelector::tripletChanged,
          this, &MainGUI::onTripletChanged);
}


void
MainGUI::createActions()
{
  loadFontsAct_ = new QAction(tr("&Load Fonts"), this);
  loadFontsAct_->setShortcuts(QKeySequence::Open);
  connect(loadFontsAct_, &QAction::triggered, this, &MainGUI::loadFonts);

  closeFontAct_ = new QAction(tr("&Close Font"), this);
  closeFontAct_->setShortcuts(QKeySequence::Close);
  connect(closeFontAct_, &QAction::triggered,
          tripletSelector_, &TripletSelector::closeCurrentFont);

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
MainGUI::setupDragDrop()
{
  setAcceptDrops(true);
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
