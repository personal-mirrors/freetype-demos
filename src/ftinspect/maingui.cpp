// maingui.cpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#include "maingui.hpp"
#include "rendering/grid.hpp"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

#include <freetype/ftdriver.h>


MainGUI::MainGUI()
{
  engine_ = NULL;

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


void
MainGUI::update(Engine* e)
{
  if (engine_)
    disconnect(&engine_->fontFileManager(), &FontFileManager::currentFileChanged,
        this, &MainGUI::watchCurrentFont);

  engine_ = e;
  connect(&engine_->fontFileManager(), &FontFileManager::currentFileChanged,
          this, &MainGUI::watchCurrentFont);
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
  checkHinting();
  adjustGlyphIndex(0);

  drawGlyph();
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

  engine_->setHinting(hintingCheckBox_->isChecked());
  engine_->setAutoHinting(autoHintingCheckBox_->isChecked());
  engine_->setHorizontalHinting(horizontalHintingCheckBox_->isChecked());
  engine_->setVerticalHinting(verticalHintingCheckBox_->isChecked());
  engine_->setBlueZoneHinting(blueZoneHintingCheckBox_->isChecked());
  engine_->setShowSegments(segmentDrawingCheckBox_->isChecked());

  engine_->setGamma(gammaSlider_->value());

  engine_->setAntiAliasingMode(static_cast<Engine::AntiAliasing>(
      antiAliasingComboBoxx_->currentIndex()));
}


void
MainGUI::clearStatusBar()
{
  statusBar()->clearMessage();
  statusBar()->setStyleSheet("");
}


void
MainGUI::checkHinting()
{
  if (hintingCheckBox_->isChecked())
  {
    if (engine_->currentFontType() == Engine::FontType_CFF)
    {
      for (int i = 0; i < hintingModeComboBoxx_->count(); i++)
      {
        if (hintingModesCFFHash_.key(i, -1) != -1)
          hintingModeComboBoxx_->setItemEnabled(i, true);
        else
          hintingModeComboBoxx_->setItemEnabled(i, false);
      }

      hintingModeComboBoxx_->setCurrentIndex(currentCFFHintingMode_);
    }
    else if (engine_->currentFontType() == Engine::FontType_TrueType)
    {
      for (int i = 0; i < hintingModeComboBoxx_->count(); i++)
      {
        if (hintingModesTrueTypeHash_.key(i, -1) != -1)
          hintingModeComboBoxx_->setItemEnabled(i, true);
        else
          hintingModeComboBoxx_->setItemEnabled(i, false);
      }

      hintingModeComboBoxx_->setCurrentIndex(currentTTInterpreterVersion_);
    }
    else
    {
      hintingModeLabel_->setEnabled(false);
      hintingModeComboBoxx_->setEnabled(false);
    }

    for (int i = 0; i < hintingModesAlwaysDisabled_.size(); i++)
      hintingModeComboBoxx_->setItemEnabled(hintingModesAlwaysDisabled_[i],
                                           false);

    autoHintingCheckBox_->setEnabled(true);
    checkAutoHinting();
  }
  else
  {
    hintingModeLabel_->setEnabled(false);
    hintingModeComboBoxx_->setEnabled(false);

    autoHintingCheckBox_->setEnabled(false);
    horizontalHintingCheckBox_->setEnabled(false);
    verticalHintingCheckBox_->setEnabled(false);
    blueZoneHintingCheckBox_->setEnabled(false);
    segmentDrawingCheckBox_->setEnabled(false);

    antiAliasingComboBoxx_->setItemEnabled(Engine::AntiAliasing_Light, false);
  }

  drawGlyph();
}


void
MainGUI::checkHintingMode()
{
  int index = hintingModeComboBoxx_->currentIndex();

  if (engine_->currentFontType() == Engine::FontType_CFF)
  {
    engine_->setCFFHintingMode(hintingModesCFFHash_.key(index));
    currentCFFHintingMode_ = index;
  }
  else if (engine_->currentFontType() == Engine::FontType_TrueType)
  {
    engine_->setTTInterpreterVersion(hintingModesTrueTypeHash_.key(index));
    currentTTInterpreterVersion_ = index;
  }

  // this enforces reloading of the font
  showFont();
}


void
MainGUI::checkAutoHinting()
{
  if (autoHintingCheckBox_->isChecked())
  {
    hintingModeLabel_->setEnabled(false);
    hintingModeComboBoxx_->setEnabled(false);

    horizontalHintingCheckBox_->setEnabled(true);
    verticalHintingCheckBox_->setEnabled(true);
    blueZoneHintingCheckBox_->setEnabled(true);
    segmentDrawingCheckBox_->setEnabled(true);

    antiAliasingComboBoxx_->setItemEnabled(Engine::AntiAliasing_Light, true);
  }
  else
  {
    if (engine_->currentFontType() == Engine::FontType_CFF
        || engine_->currentFontType() == Engine::FontType_TrueType)
    {
      hintingModeLabel_->setEnabled(true);
      hintingModeComboBoxx_->setEnabled(true);
    }

    horizontalHintingCheckBox_->setEnabled(false);
    verticalHintingCheckBox_->setEnabled(false);
    blueZoneHintingCheckBox_->setEnabled(false);
    segmentDrawingCheckBox_->setEnabled(false);

    antiAliasingComboBoxx_->setItemEnabled(Engine::AntiAliasing_Light, false);

    if (antiAliasingComboBoxx_->currentIndex() == Engine::AntiAliasing_Light)
      antiAliasingComboBoxx_->setCurrentIndex(Engine::AntiAliasing_Normal);
  }

  drawGlyph();
}


void
MainGUI::checkAntiAliasing()
{
  int index = antiAliasingComboBoxx_->currentIndex();

  if (index == Engine::AntiAliasing_None
      || index == Engine::AntiAliasing::AntiAliasing_Normal
      || index == Engine::AntiAliasing_Light)
  {
    lcdFilterLabel_->setEnabled(false);
    lcdFilterComboBox_->setEnabled(false);
  }
  else
  {
    lcdFilterLabel_->setEnabled(true);
    lcdFilterComboBox_->setEnabled(true);
  }

  drawGlyph();
}


void
MainGUI::checkLcdFilter()
{
  int index = lcdFilterComboBox_->currentIndex();
  engine_->setLcdFilter(lcdFilterHash_.key(index));
}


void
MainGUI::checkShowPoints()
{
  if (showPointsCheckBox_->isChecked())
    showPointNumbersCheckBox_->setEnabled(true);
  else
    showPointNumbersCheckBox_->setEnabled(false);

  drawGlyph();
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
    if (showBitmapCheckBox_->isChecked())
    {
      // XXX support LCD
      FT_Pixel_Mode pixelMode = FT_PIXEL_MODE_GRAY;
      if (antiAliasingComboBoxx_->currentIndex() == Engine::AntiAliasing_None)
        pixelMode = FT_PIXEL_MODE_MONO;

      currentGlyphBitmapItem_ = new GlyphBitmap(outline,
                                               engine_->ftLibrary(),
                                               pixelMode,
                                               monoColorTable_,
                                               grayColorTable_);
      glyphScene_->addItem(currentGlyphBitmapItem_);
    }

    if (showOutlinesCheckBox_->isChecked())
    {
      currentGlyphOutlineItem_ = new GlyphOutline(outlinePen_, outline);
      glyphScene_->addItem(currentGlyphOutlineItem_);
    }

    if (showPointsCheckBox_->isChecked())
    {
      currentGlyphPointsItem_ = new GlyphPoints(onPen_, offPen_, outline);
      glyphScene_->addItem(currentGlyphPointsItem_);

      if (showPointNumbersCheckBox_->isChecked())
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
  fontFilenameLabel_ = new QLabel;

  hintingCheckBox_ = new QCheckBox(tr("Hinting"));

  hintingModeLabel_ = new QLabel(tr("Hinting Mode"));
  hintingModeLabel_->setAlignment(Qt::AlignRight);
  hintingModeComboBoxx_ = new QComboBoxx;
  hintingModeComboBoxx_->insertItem(HintingMode_TrueType_v35,
                                   tr("TrueType v35"));
  hintingModeComboBoxx_->insertItem(HintingMode_TrueType_v38,
                                   tr("TrueType v38"));
  hintingModeComboBoxx_->insertItem(HintingMode_TrueType_v40,
                                   tr("TrueType v40"));
  hintingModeComboBoxx_->insertItem(HintingMode_CFF_FreeType,
                                   tr("CFF (FreeType)"));
  hintingModeComboBoxx_->insertItem(HintingMode_CFF_Adobe,
                                   tr("CFF (Adobe)"));
  hintingModeLabel_->setBuddy(hintingModeComboBoxx_);

  autoHintingCheckBox_ = new QCheckBox(tr("Auto-Hinting"));
  horizontalHintingCheckBox_ = new QCheckBox(tr("Horizontal Hinting"));
  verticalHintingCheckBox_ = new QCheckBox(tr("Vertical Hinting"));
  blueZoneHintingCheckBox_ = new QCheckBox(tr("Blue-Zone Hinting"));
  segmentDrawingCheckBox_ = new QCheckBox(tr("Segment Drawing"));

  antiAliasingLabel_ = new QLabel(tr("Anti-Aliasing"));
  antiAliasingLabel_->setAlignment(Qt::AlignRight);
  antiAliasingComboBoxx_ = new QComboBoxx;
  antiAliasingComboBoxx_->insertItem(Engine::AntiAliasing_None,
                                    tr("None"));
  antiAliasingComboBoxx_->insertItem(Engine::AntiAliasing_Normal,
                                    tr("Normal"));
  antiAliasingComboBoxx_->insertItem(Engine::AntiAliasing_Light,
                                    tr("Light"));
  antiAliasingComboBoxx_->insertItem(Engine::AntiAliasing_LCD,
                                    tr("LCD (RGB)"));
  antiAliasingComboBoxx_->insertItem(Engine::AntiAliasing_LCD_BGR,
                                    tr("LCD (BGR)"));
  antiAliasingComboBoxx_->insertItem(Engine::AntiAliasing_LCD_Vertical,
                                    tr("LCD (vert. RGB)"));
  antiAliasingComboBoxx_->insertItem(Engine::AntiAliasing_LCD_Vertical_BGR,
                                    tr("LCD (vert. BGR)"));
  antiAliasingLabel_->setBuddy(antiAliasingComboBoxx_);

  lcdFilterLabel_ = new QLabel(tr("LCD Filter"));
  lcdFilterLabel_->setAlignment(Qt::AlignRight);
  lcdFilterComboBox_ = new QComboBox;
  lcdFilterComboBox_->insertItem(LCDFilter_Default, tr("Default"));
  lcdFilterComboBox_->insertItem(LCDFilter_Light, tr("Light"));
  lcdFilterComboBox_->insertItem(LCDFilter_None, tr("None"));
  lcdFilterComboBox_->insertItem(LCDFilter_Legacy, tr("Legacy"));
  lcdFilterLabel_->setBuddy(lcdFilterComboBox_);

  int width;
  // make all labels have the same width
  width = hintingModeLabel_->minimumSizeHint().width();
  width = qMax(antiAliasingLabel_->minimumSizeHint().width(), width);
  width = qMax(lcdFilterLabel_->minimumSizeHint().width(), width);
  hintingModeLabel_->setMinimumWidth(width);
  antiAliasingLabel_->setMinimumWidth(width);
  lcdFilterLabel_->setMinimumWidth(width);

  // ensure that all items in combo boxes fit completely;
  // also make all combo boxes have the same width
  width = hintingModeComboBoxx_->minimumSizeHint().width();
  width = qMax(antiAliasingComboBoxx_->minimumSizeHint().width(), width);
  width = qMax(lcdFilterComboBox_->minimumSizeHint().width(), width);
  hintingModeComboBoxx_->setMinimumWidth(width);
  antiAliasingComboBoxx_->setMinimumWidth(width);
  lcdFilterComboBox_->setMinimumWidth(width);

  gammaLabel_ = new QLabel(tr("Gamma"));
  gammaLabel_->setAlignment(Qt::AlignRight);
  gammaSlider_ = new QSlider(Qt::Horizontal);
  gammaSlider_->setRange(0, 30); // in 1/10th
  gammaSlider_->setTickPosition(QSlider::TicksBelow);
  gammaSlider_->setTickInterval(5);
  gammaLabel_->setBuddy(gammaSlider_);

  showBitmapCheckBox_ = new QCheckBox(tr("Show Bitmap"));
  showPointsCheckBox_ = new QCheckBox(tr("Show Points"));
  showPointNumbersCheckBox_ = new QCheckBox(tr("Show Point Numbers"));
  showOutlinesCheckBox_ = new QCheckBox(tr("Show Outlines"));

  infoLeftLayout_ = new QHBoxLayout;
  infoLeftLayout_->addWidget(fontFilenameLabel_);

  hintingModeLayout_ = new QHBoxLayout;
  hintingModeLayout_->addWidget(hintingModeLabel_);
  hintingModeLayout_->addWidget(hintingModeComboBoxx_);

  horizontalHintingLayout_ = new QHBoxLayout;
  horizontalHintingLayout_->addSpacing(20); // XXX px
  horizontalHintingLayout_->addWidget(horizontalHintingCheckBox_);

  verticalHintingLayout_ = new QHBoxLayout;
  verticalHintingLayout_->addSpacing(20); // XXX px
  verticalHintingLayout_->addWidget(verticalHintingCheckBox_);

  blueZoneHintingLayout_ = new QHBoxLayout;
  blueZoneHintingLayout_->addSpacing(20); // XXX px
  blueZoneHintingLayout_->addWidget(blueZoneHintingCheckBox_);

  segmentDrawingLayout_ = new QHBoxLayout;
  segmentDrawingLayout_->addSpacing(20); // XXX px
  segmentDrawingLayout_->addWidget(segmentDrawingCheckBox_);

  antiAliasingLayout_ = new QHBoxLayout;
  antiAliasingLayout_->addWidget(antiAliasingLabel_);
  antiAliasingLayout_->addWidget(antiAliasingComboBoxx_);

  lcdFilterLayout_ = new QHBoxLayout;
  lcdFilterLayout_->addWidget(lcdFilterLabel_);
  lcdFilterLayout_->addWidget(lcdFilterComboBox_);

  gammaLayout_ = new QHBoxLayout;
  gammaLayout_->addWidget(gammaLabel_);
  gammaLayout_->addWidget(gammaSlider_);

  pointNumbersLayout_ = new QHBoxLayout;
  pointNumbersLayout_->addSpacing(20); // XXX px
  pointNumbersLayout_->addWidget(showPointNumbersCheckBox_);

  generalTabLayout_ = new QVBoxLayout;
  generalTabLayout_->addWidget(hintingCheckBox_);
  generalTabLayout_->addLayout(hintingModeLayout_);
  generalTabLayout_->addWidget(autoHintingCheckBox_);
  generalTabLayout_->addLayout(horizontalHintingLayout_);
  generalTabLayout_->addLayout(verticalHintingLayout_);
  generalTabLayout_->addLayout(blueZoneHintingLayout_);
  generalTabLayout_->addLayout(segmentDrawingLayout_);
  generalTabLayout_->addSpacing(20); // XXX px
  generalTabLayout_->addStretch(1);
  generalTabLayout_->addLayout(antiAliasingLayout_);
  generalTabLayout_->addLayout(lcdFilterLayout_);
  generalTabLayout_->addSpacing(20); // XXX px
  generalTabLayout_->addStretch(1);
  generalTabLayout_->addLayout(gammaLayout_);
  generalTabLayout_->addSpacing(20); // XXX px
  generalTabLayout_->addStretch(1);
  generalTabLayout_->addWidget(showBitmapCheckBox_);
  generalTabLayout_->addWidget(showPointsCheckBox_);
  generalTabLayout_->addLayout(pointNumbersLayout_);
  generalTabLayout_->addWidget(showOutlinesCheckBox_);

  generalTabWidget_ = new QWidget;
  generalTabWidget_->setLayout(generalTabLayout_);

  mmgxTabWidget_ = new QWidget;

  tabWidget_ = new QTabWidget;
  tabWidget_->addTab(generalTabWidget_, tr("General"));
  tabWidget_->addTab(mmgxTabWidget_, tr("MM/GX"));

  leftLayout_ = new QVBoxLayout;
  leftLayout_->addLayout(infoLeftLayout_);
  leftLayout_->addWidget(tabWidget_);

  // we don't want to expand the left side horizontally;
  // to change the policy we have to use a widget wrapper
  leftWidget_ = new QWidget;
  leftWidget_->setLayout(leftLayout_);

  QSizePolicy leftWidgetPolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  leftWidgetPolicy.setHorizontalStretch(0);
  leftWidgetPolicy.setVerticalPolicy(leftWidget_->sizePolicy().verticalPolicy());
  leftWidgetPolicy.setHeightForWidth(leftWidget_->sizePolicy().hasHeightForWidth());

  leftWidget_->setSizePolicy(leftWidgetPolicy);

  // right side
  glyphIndexLabel_ = new QLabel;
  glyphNameLabel_ = new QLabel;
  fontNameLabel_ = new QLabel;

  glyphScene_ = new QGraphicsScene;
  glyphScene_->addItem(new Grid(gridPen_, axisPen_));

  currentGlyphBitmapItem_ = NULL;
  currentGlyphOutlineItem_ = NULL;
  currentGlyphPointsItem_ = NULL;
  currentGlyphPointNumbersItem_ = NULL;
  drawGlyph();

  glyphView_ = new QGraphicsViewx;
  glyphView_->setRenderHint(QPainter::Antialiasing, true);
  glyphView_->setDragMode(QGraphicsView::ScrollHandDrag);
  glyphView_->setOptimizationFlags(QGraphicsView::DontSavePainterState);
  glyphView_->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
  glyphView_->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  glyphView_->setScene(glyphScene_);

  sizeLabel_ = new QLabel(tr("Size "));
  sizeLabel_->setAlignment(Qt::AlignRight);
  sizeDoubleSpinBox_ = new QDoubleSpinBox;
  sizeDoubleSpinBox_->setAlignment(Qt::AlignRight);
  sizeDoubleSpinBox_->setDecimals(1);
  sizeDoubleSpinBox_->setRange(1, 500);
  sizeLabel_->setBuddy(sizeDoubleSpinBox_);

  unitsComboBox_ = new QComboBox;
  unitsComboBox_->insertItem(Units_px, "px");
  unitsComboBox_->insertItem(Units_pt, "pt");

  dpiLabel_ = new QLabel(tr("DPI "));
  dpiLabel_->setAlignment(Qt::AlignRight);
  dpiSpinBox_ = new QSpinBox;
  dpiSpinBox_->setAlignment(Qt::AlignRight);
  dpiSpinBox_->setRange(10, 600);
  dpiLabel_->setBuddy(dpiSpinBox_);

  toStartButtonx_ = new QPushButtonx("|<");
  toM1000Buttonx_ = new QPushButtonx("-1000");
  toM100Buttonx_ = new QPushButtonx("-100");
  toM10Buttonx_ = new QPushButtonx("-10");
  toM1Buttonx_ = new QPushButtonx("-1");
  toP1Buttonx_ = new QPushButtonx("+1");
  toP10Buttonx_ = new QPushButtonx("+10");
  toP100Buttonx_ = new QPushButtonx("+100");
  toP1000Buttonx_ = new QPushButtonx("+1000");
  toEndButtonx_ = new QPushButtonx(">|");

  zoomLabel_ = new QLabel(tr("Zoom Factor"));
  zoomLabel_->setAlignment(Qt::AlignRight);
  zoomSpinBox_ = new QSpinBoxx;
  zoomSpinBox_->setAlignment(Qt::AlignRight);
  zoomSpinBox_->setRange(1, 1000 - 1000 % 64);
  zoomSpinBox_->setKeyboardTracking(false);
  zoomLabel_->setBuddy(zoomSpinBox_);

  previousFontButton_ = new QPushButton(tr("Previous Font"));
  nextFontButton_ = new QPushButton(tr("Next Font"));
  previousFaceButton_ = new QPushButton(tr("Previous Face"));
  nextFaceButton_ = new QPushButton(tr("Next Face"));
  previousNamedInstanceButton_ = new QPushButton(tr("Previous Named Instance"));
  nextNamedInstanceButton_ = new QPushButton(tr("Next Named Instance"));

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
  rightWidget_ = new QWidget;
  rightWidget_->setLayout(rightLayout_);

  // the whole thing
  ftinspectLayout_ = new QHBoxLayout;
  ftinspectLayout_->addWidget(leftWidget_);
  ftinspectLayout_->addWidget(rightWidget_);

  ftinspectWidget_ = new QWidget;
  ftinspectWidget_->setLayout(ftinspectLayout_);
  setCentralWidget(ftinspectWidget_);
  setWindowTitle("ftinspect");
}


void
MainGUI::createConnections()
{
  connect(hintingCheckBox_, SIGNAL(clicked()),
          SLOT(checkHinting()));

  connect(hintingModeComboBoxx_, SIGNAL(currentIndexChanged(int)),
          SLOT(checkHintingMode()));
  connect(antiAliasingComboBoxx_, SIGNAL(currentIndexChanged(int)),
          SLOT(checkAntiAliasing()));
  connect(lcdFilterComboBox_, SIGNAL(currentIndexChanged(int)),
          SLOT(checkLcdFilter()));

  connect(autoHintingCheckBox_, SIGNAL(clicked()),
          SLOT(checkAutoHinting()));
  connect(showBitmapCheckBox_, SIGNAL(clicked()),
          SLOT(drawGlyph()));
  connect(showPointsCheckBox_, SIGNAL(clicked()),
          SLOT(checkShowPoints()));
  connect(showPointNumbersCheckBox_, SIGNAL(clicked()),
          SLOT(drawGlyph()));
  connect(showOutlinesCheckBox_, SIGNAL(clicked()),
          SLOT(drawGlyph()));

  connect(sizeDoubleSpinBox_, SIGNAL(valueChanged(double)),
          SLOT(drawGlyph()));
  connect(unitsComboBox_, SIGNAL(currentIndexChanged(int)),
          SLOT(checkUnits()));
  connect(dpiSpinBox_, SIGNAL(valueChanged(int)),
          SLOT(drawGlyph()));

  connect(zoomSpinBox_, SIGNAL(valueChanged(int)),
          SLOT(zoom()));

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
  // set up mappings between property values and combo box indices
  hintingModesTrueTypeHash_[TT_INTERPRETER_VERSION_35] = HintingMode_TrueType_v35;
  hintingModesTrueTypeHash_[TT_INTERPRETER_VERSION_38] = HintingMode_TrueType_v38;
  hintingModesTrueTypeHash_[TT_INTERPRETER_VERSION_40] = HintingMode_TrueType_v40;

  hintingModesCFFHash_[FT_HINTING_FREETYPE] = HintingMode_CFF_FreeType;
  hintingModesCFFHash_[FT_HINTING_ADOBE] = HintingMode_CFF_Adobe;

  lcdFilterHash_[FT_LCD_FILTER_DEFAULT] = LCDFilter_Default;
  lcdFilterHash_[FT_LCD_FILTER_LIGHT] = LCDFilter_Light;
  lcdFilterHash_[FT_LCD_FILTER_NONE] = LCDFilter_None;
  lcdFilterHash_[FT_LCD_FILTER_LEGACY] = LCDFilter_Legacy;

  Engine::EngineDefaultValues& defaults = engine_->engineDefaults();

  // make copies and remove existing elements...
  QHash<int, int> hmTTHash = hintingModesTrueTypeHash_;
  if (hmTTHash.contains(defaults.ttInterpreterVersionDefault))
    hmTTHash.remove(defaults.ttInterpreterVersionDefault);
  if (hmTTHash.contains(defaults.ttInterpreterVersionOther))
    hmTTHash.remove(defaults.ttInterpreterVersionOther);
  if (hmTTHash.contains(defaults.ttInterpreterVersionOther1))
    hmTTHash.remove(defaults.ttInterpreterVersionOther1);

  QHash<int, int> hmCFFHash = hintingModesCFFHash_;
  if (hmCFFHash.contains(defaults.cffHintingEngineDefault))
    hmCFFHash.remove(defaults.cffHintingEngineDefault);
  if (hmCFFHash.contains(defaults.cffHintingEngineOther))
    hmCFFHash.remove(defaults.cffHintingEngineOther);

  // ... to construct a list of always disabled hinting mode combo box items
  hintingModesAlwaysDisabled_ = hmTTHash.values();
  hintingModesAlwaysDisabled_ += hmCFFHash.values();

  for (int i = 0; i < hintingModesAlwaysDisabled_.size(); i++)
    hintingModeComboBoxx_->setItemEnabled(hintingModesAlwaysDisabled_[i],
                                         false);

  // the next four values always non-negative
  currentFontIndex_ = 0;
  currentFaceIndex_ = 0;
  currentNamedInstanceIndex_ = 0;
  currentGlyphIndex_ = 0;

  currentCFFHintingMode_
    = hintingModesCFFHash_[defaults.cffHintingEngineDefault];
  currentTTInterpreterVersion_
    = hintingModesTrueTypeHash_[defaults.ttInterpreterVersionDefault];

  hintingCheckBox_->setChecked(true);

  antiAliasingComboBoxx_->setCurrentIndex(Engine::AntiAliasing_Normal);
  lcdFilterComboBox_->setCurrentIndex(LCDFilter_Light);

  horizontalHintingCheckBox_->setChecked(true);
  verticalHintingCheckBox_->setChecked(true);
  blueZoneHintingCheckBox_->setChecked(true);

  showBitmapCheckBox_->setChecked(true);
  showOutlinesCheckBox_->setChecked(true);

  gammaSlider_->setValue(18); // 1.8
  sizeDoubleSpinBox_->setValue(20);
  dpiSpinBox_->setValue(96);
  zoomSpinBox_->setValue(20);

  checkHinting();
  checkHintingMode();
  checkAutoHinting();
  checkAntiAliasing();
  checkLcdFilter();
  checkShowPoints();
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
