// maingui.cpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#include "maingui.hpp"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QScrollBar>
#include <QStatusBar>


MainGUI::MainGUI(Engine* engine)
: engine_(engine)
{
  createLayout();
  createConnections();
  createActions();
  createMenus();

  readSettings();
  setUnifiedTitleAndToolBarOnMac(true);

  show();
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
MainGUI::keyPressEvent(QKeyEvent* event)
{
  // Delegate key events to tabs
  if (!tabWidget_->currentWidget()->eventFilter(this, event))
    QMainWindow::keyPressEvent(event);
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
  engine_->openFonts(fileNames);
  tripletSelector_->repopulateFonts();
}


void
MainGUI::onTripletChanged()
{
  auto state = settingPanel_->blockSignals(true);
  settingPanel_->onFontChanged();
  settingPanel_->blockSignals(state);
  reloadCurrentTabFont();
}


void
MainGUI::switchTab()
{
  reloadCurrentTabFont();
  lastTab_ = tabWidget_->currentWidget();
}


void
MainGUI::repaintCurrentTab()
{
  applySettings();
  tabs_[tabWidget_->currentIndex()]->repaintGlyph();
}


void
MainGUI::reloadCurrentTabFont()
{
  engine_->resetCache();
  applySettings();
  auto index = tabWidget_->currentIndex();
  if (index >= 0 && static_cast<size_t>(index) < tabs_.size())
    tabs_[index]->reloadFont();
}


void
MainGUI::applySettings()
{
  settingPanel_->applySettings();
  settingPanel_->applyDelayedSettings();
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
  leftLayout_->setContentsMargins(32, 32, 0, 16);

  // we don't want to expand the left side horizontally;
  // to change the policy we have to use a widget wrapper
  leftWidget_ = new QWidget(this);
  leftWidget_->setLayout(leftLayout_);
  leftWidget_->setMaximumWidth(400);

  // right side
  // TODO: create tabs here

  tabWidget_ = new QTabWidget(this);
  tabWidget_->setObjectName("mainTab"); // for stylesheet

  // Note those two list must be in sync
  // TODO: add tabs and tooltips here
  
  tripletSelector_ = new TripletSelector(this, engine_);

  rightLayout_ = new QVBoxLayout;
  //rightLayout_->addWidget(fontNameLabel_);
  rightLayout_->addWidget(tabWidget_); // same for `leftLayout_`: Remove?
  rightLayout_->setContentsMargins(16, 32, 32, 16);

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

  ftinspectLayout_->setSizeConstraint(QLayout::SetNoConstraint);
  layout()->setSizeConstraint(QLayout::SetNoConstraint);
  resize(ftinspectWidget_->minimumSizeHint().width(),
         700 * logicalDpiY() / 96);

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
          this, &MainGUI::switchTab);

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
