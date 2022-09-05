// maingui.cpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#include "maingui.hpp"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QScrollBar>
#include <QStatusBar>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>


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

  show(); // place this before `loadCommandLine` so alerts from loading
          // won't be covered.
  loadCommandLine();
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
MainGUI::loadCommandLine()
{
  engine_->fontFileManager().loadFromCommandLine();
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
  auto current = tabWidget_->currentWidget();
  auto isComparator = current == comparatorTab_;

  if (isComparator)
    tabWidget_->setStyleSheet(
      QString("QTabWidget#mainTab::tab-bar {left: %1 px;}")
      .arg(leftWidget_->width()));
  else
    tabWidget_->setStyleSheet("");
  

  if (!leftWidget_->isVisible() && !isComparator)
  {
    // Dirty approach here: When setting the left panel as visible, the main
    // window will auto-expand. However, we don't want this behaviour.
    // Doing `resize` right after the `setVisible` is useless since the
    // layout updating is delayed, so we have to temporarily fix the main window
    // size, and recover the original min/max size when finished.
    auto minSize = minimumSize();
    auto maxSize = maximumSize();
    setFixedSize(size());
    leftWidget_->setVisible(true);
    setMinimumSize(minSize);
    setMaximumSize(maxSize);
  }
  else
    leftWidget_->setVisible(!isComparator);

  reloadCurrentTabFont();

  if (current == continuousTab_ && lastTab_ == singularTab_
      && singularTab_->currentGlyph() >= 0)
    continuousTab_->highlightGlyph(singularTab_->currentGlyph());

  lastTab_ = current;
}


void
MainGUI::switchToSingular(int glyphIndex,
                          double sizePoint)
{
  tabWidget_->setCurrentWidget(singularTab_); // this would update the tab
  singularTab_->setCurrentGlyphAndSize(glyphIndex, sizePoint);
}


void
MainGUI::closeDockWidget()
{
  glyphDetailsDockWidget_->hide();
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
  if (tabWidget_->currentWidget() != comparatorTab_)
    settingPanel_->applyDelayedSettings(); // This will reset the cache.
  applySettings();
  auto index = tabWidget_->currentIndex();
  if (index >= 0 && static_cast<size_t>(index) < tabs_.size())
    tabs_[index]->reloadFont();
}


void
MainGUI::applySettings()
{
  if (tabWidget_->currentWidget() != comparatorTab_)
    settingPanel_->applySettings();
}


// XXX distances are specified in pixels,
//     making the layout dependent on the output device resolution
void
MainGUI::createLayout()
{
  // floating
  glyphDetailsDockWidget_ = new QDockWidget(tr("Glyph Details"), this);
  glyphDetails_ = new GlyphDetails(glyphDetailsDockWidget_, engine_);
  glyphDetailsDockWidget_->setWidget(glyphDetails_);
  glyphDetailsDockWidget_->setFloating(true);
  glyphDetailsDockWidget_->hide();

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
  singularTab_ = new SingularTab(this, engine_);
  continuousTab_ = new ContinuousTab(this, engine_,
                                     glyphDetailsDockWidget_, glyphDetails_);
  comparatorTab_ = new ComparatorTab(this, engine_);

  tabWidget_ = new QTabWidget(this);
  tabWidget_->setObjectName("mainTab"); // for stylesheet

  // Note those two list must be in sync
  tabs_.push_back(singularTab_);
  tabWidget_->addTab(singularTab_, tr("Singular Grid View"));
  tabs_.push_back(continuousTab_);
  tabWidget_->addTab(continuousTab_, tr("Continuous View"));
  tabs_.push_back(comparatorTab_);
  tabWidget_->addTab(comparatorTab_, tr("Comparator View"));
  lastTab_ = singularTab_;
  
  tabWidget_->setTabToolTip(0, tr("View single glyph in grid view.\n"
                                  "For pixelwise inspection of the glyphs."));
  tabWidget_->setTabToolTip(1, tr("View a string of glyphs continuously.\n"
                                  "Show all glyphs in the font or render "
                                  "strings."));
  tabWidget_->setTabToolTip(2, tr("Compare the output of the font "
                                  "in different rendering settings "
                                  "(e.g. hintings)."));
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

  connect(continuousTab_, &ContinuousTab::switchToSingular,
          this, &MainGUI::switchToSingular);
  connect(glyphDetails_, &GlyphDetails::closeDockWidget, 
          this, &MainGUI::closeDockWidget);
  connect(glyphDetails_, &GlyphDetails::switchToSingular,
          [&] (int index)
          {
            switchToSingular(index, -1);
            if (glyphDetailsDockWidget_->isFloating())
              glyphDetailsDockWidget_->hide();
          });
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
