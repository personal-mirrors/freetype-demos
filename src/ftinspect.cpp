// ftinspect.cpp

// Copyright (C) 2016 by Werner Lemberg.

#include "ftinspect.h"


#define VERSION "X.Y.Z"


FaceID::FaceID()
: fontIndex(0),
  faceIndex(0),
  instanceIndex(0)
{
  // empty
}


FaceID::FaceID(int fontIdx,
               int faceIdx,
               int instanceIdx)
: fontIndex(fontIdx),
  faceIndex(faceIdx),
  instanceIndex(instanceIdx)
{
  // empty
}


bool
FaceID::operator==(const FaceID& other) const
{
  return (fontIndex == other.fontIndex
          && faceIndex == other.faceIndex
          && instanceIndex == other.instanceIndex);
}


uint
qHash(FaceID key)
{
  return ((uint)key.fontIndex << 20)
         | ((uint)key.faceIndex << 10)
         | (uint)key.instanceIndex;
}


// The face requester is a function provided by the client application to
// the cache manager to translate an `abstract' face ID into a real
// `FT_Face' object.
//
// We use a hash: `faceID' is the value, and its associated key gives the
// font, face, and instance indices.  Getting a key from a value is slow,
// but this must be done only once.

FT_Error
faceRequester(FTC_FaceID faceID,
              FT_Library library,
              FT_Pointer requestData,
              FT_Face* faceP)
{
  MainGUI* gui = static_cast<MainGUI*>(requestData);
  // in C++ it's tricky to convert a void pointer back to an integer
  // without warnings related to 32bit vs. 64bit pointer size
  int val = static_cast<int>((char*)faceID - (char*)0);
  const FaceID& id = gui->faceIDHash.key(val);

  Font& font = gui->fonts[id.fontIndex];
  int faceIndex = id.faceIndex;

  if (id.instanceIndex >= 0)
    faceIndex += id.instanceIndex << 16;

  return FT_New_Face(library,
                     qPrintable(font.filePathname),
                     faceIndex,
                     faceP);
}


Engine::Engine(MainGUI* g)
{
  gui = g;

  FT_Error error;

  error = FT_Init_FreeType(&library);
  if (error)
  {
    // XXX error handling
  }

  error = FTC_Manager_New(library, 0, 0, 0,
                          faceRequester, gui, &cacheManager);
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

  // query engines and check for alternatives

  // CFF
  error = FT_Property_Get(library,
                          "cff",
                          "hinting-engine",
                          &cffHintingEngineDefault);
  if (error)
  {
    // no CFF engine
    cffHintingEngineDefault = -1;
    cffHintingEngineOther = -1;
  }
  else
  {
    int engines[2] =
    {
      FT_CFF_HINTING_FREETYPE,
      FT_CFF_HINTING_ADOBE
    };

    int i;
    for (i = 0; i < 2; i++)
      if (cffHintingEngineDefault == engines[i])
        break;

    cffHintingEngineOther = engines[(i + 1) % 2];

    error = FT_Property_Set(library,
                            "cff",
                            "hinting-engine",
                            &cffHintingEngineOther);
    if (error)
      cffHintingEngineOther = -1;

    // reset
    FT_Property_Set(library,
                    "cff",
                    "hinting-engine",
                    &cffHintingEngineDefault);
  }

  // TrueType
  error = FT_Property_Get(library,
                          "truetype",
                          "interpreter-version",
                          &ttInterpreterVersionDefault);
  if (error)
  {
    // no TrueType engine
    ttInterpreterVersionDefault = -1;
    ttInterpreterVersionOther = -1;
    ttInterpreterVersionOther1 = -1;
  }
  else
  {
    int interpreters[3] =
    {
      TT_INTERPRETER_VERSION_35,
      TT_INTERPRETER_VERSION_38,
      40, // TT_INTERPRETER_VERSION_40, not yet implemented
    };

    int i;
    for (i = 0; i < 3; i++)
      if (ttInterpreterVersionDefault == interpreters[i])
        break;

    ttInterpreterVersionOther = interpreters[(i + 1) % 3];

    error = FT_Property_Set(library,
                            "truetype",
                            "interpreter-version",
                            &ttInterpreterVersionOther);
    if (error)
      ttInterpreterVersionOther = -1;

    ttInterpreterVersionOther1 = interpreters[(i + 2) % 3];

    error = FT_Property_Set(library,
                            "truetype",
                            "interpreter-version",
                            &ttInterpreterVersionOther1);
    if (error)
      ttInterpreterVersionOther1 = -1;

    // reset
    FT_Property_Set(library,
                    "truetype",
                    "interpreter-version",
                    &ttInterpreterVersionDefault);
  }

  // auto-hinter
  error = FT_Property_Get(library,
                          "autofitter",
                          "warping",
                          &doWarping);
  if (error)
  {
    // no warping
    haveWarping = 0;
    doWarping = 0;
  }
  else
  {
    haveWarping = 1;
    doWarping = 0; // we don't do warping by default

    FT_Property_Set(library,
                    "autofitter",
                    "warping",
                    &doWarping);
  }

  update();
}


Engine::~Engine()
{
  FTC_Manager_Done(cacheManager);
  FT_Done_FreeType(library);
}


int
Engine::numFaces(int fontIndex)
{
  if (fontIndex >= gui->fonts.size())
    return -1;

  Font& font = gui->fonts[fontIndex];

  // value already available?
  if (!font.numInstancesList.isEmpty())
    return font.numInstancesList.size();

  FT_Error error;
  FT_Face face;

  error = FT_New_Face(library,
                      qPrintable(font.filePathname),
                      -1,
                      &face);
  if (error)
  {
    // XXX error handling
    return -1;
  }

  int result = face->num_faces;

  FT_Done_Face(face);

  return result;
}


int
Engine::numInstances(int fontIndex,
                     int faceIndex)
{
  if (fontIndex >= gui->fonts.size())
    return -1;

  Font& font = gui->fonts[fontIndex];

  if (faceIndex >= font.numInstancesList.size())
    return -1;

  // value already available?
  if (font.numInstancesList[faceIndex] >= 0)
    return font.numInstancesList[faceIndex];

  FT_Error error;
  FT_Face face;

  error = FT_New_Face(library,
                      qPrintable(font.filePathname),
                      -(faceIndex + 1),
                      &face);
  if (error)
  {
    // XXX error handling
    return -1;
  }

  // we return `n' instances plus one,
  // the latter representing a face without an instance selected
  int result = (face->style_flags >> 16) + 1;

  FT_Done_Face(face);

  return result;
}


int
Engine::loadFont(int fontIndex,
                 int faceIndex,
                 int instanceIndex)
{
  scaler.face_id = reinterpret_cast<void*>
                     (gui->faceIDHash.value(FaceID(fontIndex,
                                                   faceIndex,
                                                   instanceIndex)));

  FT_Error error = FTC_Manager_LookupSize(cacheManager, &scaler, &ftSize);
  if (error)
  {
    // XXX error handling
    return -1;
  }

  return ftSize->face->num_glyphs;
}


void
Engine::removeFont(int fontIndex,
                   int faceIndex,
                   int instanceIndex)
{
  FTC_FaceID face_id = reinterpret_cast<void*>
                         (gui->faceIDHash.value(FaceID(fontIndex,
                                                       faceIndex,
                                                       instanceIndex)));
  FTC_Manager_RemoveFaceID(cacheManager, face_id);
}


void
Engine::update()
{
  dpi = gui->dpiSpinBox->value();
  zoom = gui->zoomSpinBox->value();

  if (gui->unitsComboBox->currentIndex() == MainGUI::Units_px)
  {
    pointSize = gui->sizeDoubleSpinBox->value();
    pixelSize = pointSize * dpi / 72.0;
  }
  else
  {
    pixelSize = gui->sizeDoubleSpinBox->value();
    pointSize = pixelSize * 72.0 / dpi;
  }

  doHinting = gui->hintingCheckBox->isChecked();

  doAutoHinting = gui->autoHintingCheckBox->isChecked();
  doHorizontalHinting = gui->horizontalHintingCheckBox->isChecked();
  doVerticalHinting = gui->verticalHintingCheckBox->isChecked();
  doBlueZoneHinting = gui->blueZoneHintingCheckBox->isChecked();
  showSegments = gui->segmentDrawingCheckBox->isChecked();
  doWarping = gui->warpingCheckBox->isChecked();

  showBitmap = gui->showBitmapCheckBox->isChecked();
  showPoints = gui->showPointsCheckBox->isChecked();
  if (showPoints)
    showPointIndices = gui->showPointIndicesCheckBox->isChecked();
  else
    showPointIndices = false;
  showOutlines = gui->showOutlinesCheckBox->isChecked();

  gamma = gui->gammaSlider->value();

  loadFlags = FT_LOAD_DEFAULT;
  if (doAutoHinting)
    loadFlags |= FT_LOAD_FORCE_AUTOHINT;
  loadFlags |= FT_LOAD_NO_BITMAP; // XXX handle bitmap fonts also

  int index = gui->antiAliasingComboBoxx->currentIndex();

  if (doHinting)
  {
    int target;

    if (index == MainGUI::AntiAliasing_None)
      target = FT_LOAD_TARGET_MONO;
    else
    {
      switch (index)
      {
      case MainGUI::AntiAliasing_Slight:
        target = FT_LOAD_TARGET_LIGHT;
        break;

      case MainGUI::AntiAliasing_LCD:
      case MainGUI::AntiAliasing_LCD_BGR:
        target = FT_LOAD_TARGET_LCD;
        break;

      case MainGUI::AntiAliasing_LCD_Vertical:
      case MainGUI::AntiAliasing_LCD_Vertical_BGR:
        target = FT_LOAD_TARGET_LCD_V;
        break;

      default:
        target = FT_LOAD_TARGET_NORMAL;
      }
    }

    loadFlags |= target;
  }
  else
  {
    loadFlags |= FT_LOAD_NO_HINTING;

    if (index == MainGUI::AntiAliasing_None)
      loadFlags |= FT_LOAD_MONOCHROME;
  }

  // XXX handle color fonts also

  scaler.pixel = 0; // use 26.6 format

  if (gui->unitsComboBox->currentIndex() == MainGUI::Units_px)
  {
    scaler.width = int(pixelSize * 64.0);
    scaler.height = int(pixelSize * 64.0);
    scaler.x_res = 0;
    scaler.y_res = 0;
  }
  else
  {
    scaler.width = int(pointSize * 64.0);
    scaler.height = int(pointSize * 64.0);
    scaler.x_res = dpi;
    scaler.y_res = dpi;
  }


}


MainGUI::MainGUI()
{
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
}


void
MainGUI::update(Engine* e)
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
  int oldSize = fonts.size();

  QStringList files = QFileDialog::getOpenFileNames(
                        this,
                        tr("Load one or more fonts"),
                        QDir::homePath(),
                        "",
                        NULL,
                        QFileDialog::ReadOnly);

  for (int i = 0; i < files.size(); i++)
  {
    Font font;
    font.filePathname = files[i];
    fonts.append(font);
  }

  // if we have new fonts, set the current index to the first new one
  if (oldSize < fonts.size())
    currentFontIndex = oldSize;

  showFont();
}


void
MainGUI::closeFont()
{
  if (currentFontIndex >= 0)
  {
    for (int i = 0; i < fonts[currentFontIndex].numInstancesList.size(); i++)
      for (int j = 0; j < fonts[currentFontIndex].numInstancesList[i]; j++)
      {
        faceIDHash.remove(FaceID(currentFontIndex, i, j));
        engine->removeFont(currentFontIndex, i, j);
      }

    fonts.removeAt(currentFontIndex);
  }
  if (currentFontIndex >= fonts.size())
    currentFontIndex--;

  if (currentFontIndex < 0
      || fonts[currentFontIndex].numInstancesList.isEmpty())
  {
    currentFaceIndex = -1;
    currentInstanceIndex = -1;
  }
  else
  {
    currentFaceIndex = 0;
    currentInstanceIndex = 0;
  }

  showFont();
}


void
MainGUI::showFont()
{
  if (currentFontIndex >= 0)
  {
    // we do lazy computation of FT_Face objects

    Font& font = fonts[currentFontIndex];

    // if not yet available, extract the number of faces and indices
    // for the current font

    if (font.numInstancesList.isEmpty())
    {
      int numFaces = engine->numFaces(currentFontIndex);

      if (numFaces > 0)
      {
        for (int i = 0; i < numFaces; i++)
          font.numInstancesList.append(-1);

        currentFaceIndex = 0;
        currentInstanceIndex = 0;
      }
      else
      {
        // we use `numInstancesList' with a single element set to zero
        // to indicate either a non-font or a font FreeType couldn't load;
        font.numInstancesList.append(0);

        currentFaceIndex = -1;
        currentInstanceIndex = -1;
      }
    }

    // value -1 in `numInstancesList' means `not yet initialized'
    if (currentFaceIndex >= 0
        && font.numInstancesList[currentFaceIndex] < 0)
    {
      int numInstances = engine->numInstances(currentFontIndex,
                                              currentFaceIndex);

      // XXX? we ignore errors
      if (numInstances < 0)
        numInstances = 1;

      font.numInstancesList[currentFaceIndex] = numInstances;

      // assign the (font,face,instance) triplet to a running ID;
      // we need this for the `faceRequester' function
      for (int i = 0; i < numInstances; i++)
        faceIDHash.insert(FaceID(currentFontIndex, currentFaceIndex, i),
                          faceCounter++);

      // instance index 0 represents a face without an instance;
      // consequently, `n' instances are enumerated from 1 to `n'
      // (instead of having indices 0 to `n-1')
      currentInstanceIndex = 0;
    }

    if (currentFaceIndex >= 0)
    {
      // up to now we only called for rudimentary font handling
      // (via the `engine->numFaces' and `engine->numInstances' methods);
      // `engine->loadFont', however, really parses a font

      // if the (font,face,instance) triplet is invalid,
      // remove it from the hash
      int currentNumGlyphs = engine->loadFont(currentFontIndex,
                                              currentFaceIndex,
                                              currentInstanceIndex);
      if (currentNumGlyphs < 0)
        faceIDHash.remove(FaceID(currentFontIndex,
                                 currentFaceIndex,
                                 currentInstanceIndex));
    }
  }

  checkCurrentFontIndex();
  checkCurrentFaceIndex();
  checkCurrentInstanceIndex();
}


void
MainGUI::checkHinting()
{
  if (hintingCheckBox->isChecked())
  {
    autoHintingCheckBox->setEnabled(true);
    checkAutoHinting();
  }
  else
  {
    hintingModeLabel->setEnabled(false);
    hintingModeComboBoxx->setEnabled(false);

    autoHintingCheckBox->setEnabled(false);
    horizontalHintingCheckBox->setEnabled(false);
    verticalHintingCheckBox->setEnabled(false);
    blueZoneHintingCheckBox->setEnabled(false);
    segmentDrawingCheckBox->setEnabled(false);
    warpingCheckBox->setEnabled(false);

    antiAliasingComboBoxx->setItemEnabled(AntiAliasing_Slight, false);
  }
}


void
MainGUI::checkHintingMode()
{
  int index = hintingModeComboBoxx->currentIndex();

  // XXX to be completed
}


void
MainGUI::checkAutoHinting()
{
  if (autoHintingCheckBox->isChecked())
  {
    hintingModeLabel->setEnabled(false);
    hintingModeComboBoxx->setEnabled(false);

    horizontalHintingCheckBox->setEnabled(true);
    verticalHintingCheckBox->setEnabled(true);
    blueZoneHintingCheckBox->setEnabled(true);
    segmentDrawingCheckBox->setEnabled(true);
    if (engine->haveWarping)
      warpingCheckBox->setEnabled(true);

    antiAliasingComboBoxx->setItemEnabled(AntiAliasing_Slight, true);
  }
  else
  {
    hintingModeLabel->setEnabled(true);
    hintingModeComboBoxx->setEnabled(true);

    horizontalHintingCheckBox->setEnabled(false);
    verticalHintingCheckBox->setEnabled(false);
    blueZoneHintingCheckBox->setEnabled(false);
    segmentDrawingCheckBox->setEnabled(false);
    warpingCheckBox->setEnabled(false);

    antiAliasingComboBoxx->setItemEnabled(AntiAliasing_Slight, false);

    if (antiAliasingComboBoxx->currentIndex() == AntiAliasing_Slight)
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
MainGUI::checkLcdFilter()
{
  int index = lcdFilterComboBox->currentIndex();
  FT_Library_SetLcdFilter(engine->library, lcdFilterHash.key(index));
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
MainGUI::checkCurrentFontIndex()
{
  if (fonts.size() < 2)
  {
    previousFontButton->setEnabled(false);
    nextFontButton->setEnabled(false);
  }
  else if (currentFontIndex == 0)
  {
    previousFontButton->setEnabled(false);
    nextFontButton->setEnabled(true);
  }
  else if (currentFontIndex == fonts.size() - 1)
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
  int numFaces;

  if (currentFontIndex < 0)
    numFaces = 0;
  else
    numFaces = fonts[currentFontIndex].numInstancesList.size();

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
MainGUI::checkCurrentInstanceIndex()
{
  int numInstances;

  if (currentFontIndex < 0)
    numInstances = 0;
  else
  {
    if (currentFaceIndex < 0)
      numInstances = 0;
    else
      numInstances = fonts[currentFontIndex]
                       .numInstancesList[currentFaceIndex];
  }

  if (numInstances < 2)
  {
    previousInstanceButton->setEnabled(false);
    nextInstanceButton->setEnabled(false);
  }
  else if (currentInstanceIndex == 0)
  {
    previousInstanceButton->setEnabled(false);
    nextInstanceButton->setEnabled(true);
  }
  else if (currentInstanceIndex == numInstances - 1)
  {
    previousInstanceButton->setEnabled(true);
    nextInstanceButton->setEnabled(false);
  }
  else
  {
    previousInstanceButton->setEnabled(true);
    nextInstanceButton->setEnabled(true);
  }
}


void
MainGUI::previousFont()
{
  if (currentFontIndex > 0)
  {
    currentFontIndex--;
    currentFaceIndex = 0;
    currentInstanceIndex = 0;
    showFont();
  }
}


void
MainGUI::nextFont()
{
  if (currentFontIndex < fonts.size() - 1)
  {
    currentFontIndex++;
    currentFaceIndex = 0;
    currentInstanceIndex = 0;
    showFont();
  }
}


void
MainGUI::previousFace()
{
  if (currentFaceIndex > 0)
  {
    currentFaceIndex--;
    currentInstanceIndex = 0;
    showFont();
  }
}


void
MainGUI::nextFace()
{
  int numFaces = fonts[currentFontIndex].numInstancesList.size();

  if (currentFaceIndex < numFaces - 1)
  {
    currentFaceIndex++;
    currentInstanceIndex = 0;
    showFont();
  }
}


void
MainGUI::previousInstance()
{
  if (currentInstanceIndex > 0)
  {
    currentInstanceIndex--;
    showFont();
  }
}


void
MainGUI::nextInstance()
{
  int numInstances = fonts[currentFontIndex]
                       .numInstancesList[currentFaceIndex];

  if (currentInstanceIndex < numInstances - 1)
  {
    currentInstanceIndex++;
    showFont();
  }
}


// XXX distances are specified in pixels,
//     making the layout dependent on the output device resolution
void
MainGUI::createLayout()
{
  // left side
  hintingCheckBox = new QCheckBox(tr("Hinting"));

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
  hintingModeLabel->setBuddy(hintingModeComboBoxx);

  autoHintingCheckBox = new QCheckBox(tr("Auto-Hinting"));
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
  gammaSlider->setTickInterval(5);
  gammaLabel->setBuddy(gammaSlider);

  showBitmapCheckBox = new QCheckBox(tr("Show Bitmap"));
  showPointsCheckBox = new QCheckBox(tr("Show Points"));
  showPointIndicesCheckBox = new QCheckBox(tr("Show Point Indices"));
  showOutlinesCheckBox = new QCheckBox(tr("Show Outlines"));

  watchButton = new QPushButton(tr("Watch"));

  hintingModeLayout = new QHBoxLayout;
  hintingModeLayout->addWidget(hintingModeLabel);
  hintingModeLayout->addWidget(hintingModeComboBoxx);

  horizontalHintingLayout = new QHBoxLayout;
  horizontalHintingLayout->addSpacing(20); // XXX px
  horizontalHintingLayout->addWidget(horizontalHintingCheckBox);

  verticalHintingLayout = new QHBoxLayout;
  verticalHintingLayout->addSpacing(20); // XXX px
  verticalHintingLayout->addWidget(verticalHintingCheckBox);

  blueZoneHintingLayout = new QHBoxLayout;
  blueZoneHintingLayout->addSpacing(20); // XXX px
  blueZoneHintingLayout->addWidget(blueZoneHintingCheckBox);

  segmentDrawingLayout = new QHBoxLayout;
  segmentDrawingLayout->addSpacing(20); // XXX px
  segmentDrawingLayout->addWidget(segmentDrawingCheckBox);

  warpingLayout = new QHBoxLayout;
  warpingLayout->addSpacing(20); // XXX px
  warpingLayout->addWidget(warpingCheckBox);

  antiAliasingLayout = new QHBoxLayout;
  antiAliasingLayout->addWidget(antiAliasingLabel);
  antiAliasingLayout->addWidget(antiAliasingComboBoxx);

  lcdFilterLayout = new QHBoxLayout;
  lcdFilterLayout->addWidget(lcdFilterLabel);
  lcdFilterLayout->addWidget(lcdFilterComboBox);

  gammaLayout = new QHBoxLayout;
  gammaLayout->addWidget(gammaLabel);
  gammaLayout->addWidget(gammaSlider);

  pointIndicesLayout = new QHBoxLayout;
  pointIndicesLayout->addSpacing(20); // XXX px
  pointIndicesLayout->addWidget(showPointIndicesCheckBox);

  generalTabLayout = new QVBoxLayout;
  generalTabLayout->addWidget(hintingCheckBox);
  generalTabLayout->addLayout(hintingModeLayout);
  generalTabLayout->addWidget(autoHintingCheckBox);
  generalTabLayout->addLayout(horizontalHintingLayout);
  generalTabLayout->addLayout(verticalHintingLayout);
  generalTabLayout->addLayout(blueZoneHintingLayout);
  generalTabLayout->addLayout(segmentDrawingLayout);
  generalTabLayout->addLayout(warpingLayout);
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
  generalTabLayout->addLayout(pointIndicesLayout);
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
  sizeLabel->setBuddy(sizeDoubleSpinBox);

  unitsComboBox = new QComboBox;
  unitsComboBox->insertItem(Units_px, "px");
  unitsComboBox->insertItem(Units_pt, "pt");

  dpiLabel = new QLabel(tr("DPI "));
  dpiLabel->setAlignment(Qt::AlignRight);
  dpiSpinBox = new QSpinBox;
  dpiSpinBox->setAlignment(Qt::AlignRight);
  dpiSpinBox->setRange(10, 600);
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
  zoomLabel->setBuddy(zoomSpinBox);

  previousFontButton = new QPushButton(tr("Previous Font"));
  nextFontButton = new QPushButton(tr("Next Font"));
  previousFaceButton = new QPushButton(tr("Previous Face"));
  nextFaceButton = new QPushButton(tr("Next Face"));
  previousInstanceButton = new QPushButton(tr("Previous Instance"));
  nextInstanceButton = new QPushButton(tr("Next Instance"));

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

  fontLayout = new QGridLayout;
  fontLayout->setColumnStretch(0, 2);
  fontLayout->addWidget(nextFontButton, 0, 1);
  fontLayout->addWidget(previousFontButton, 1, 1);
  fontLayout->setColumnStretch(2, 1);
  fontLayout->addWidget(nextFaceButton, 0, 3);
  fontLayout->addWidget(previousFaceButton, 1, 3);
  fontLayout->setColumnStretch(4, 1);
  fontLayout->addWidget(nextInstanceButton, 0, 5);
  fontLayout->addWidget(previousInstanceButton, 1, 5);
  fontLayout->setColumnStretch(6, 2);

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
  connect(hintingCheckBox, SIGNAL(clicked()),
          SLOT(checkHinting()));

  connect(hintingModeComboBoxx, SIGNAL(currentIndexChanged(int)),
          SLOT(checkHintingMode()));
  connect(antiAliasingComboBoxx, SIGNAL(currentIndexChanged(int)),
          SLOT(checkAntiAliasing()));
  connect(lcdFilterComboBox, SIGNAL(currentIndexChanged(int)),
          SLOT(checkLcdFilter()));

  connect(autoHintingCheckBox, SIGNAL(clicked()),
          SLOT(checkAutoHinting()));
  connect(showPointsCheckBox, SIGNAL(clicked()),
          SLOT(checkShowPoints()));

  connect(unitsComboBox, SIGNAL(currentIndexChanged(int)),
          SLOT(checkUnits()));

  connect(previousFontButton, SIGNAL(clicked()),
          SLOT(previousFont()));
  connect(nextFontButton, SIGNAL(clicked()),
          SLOT(nextFont()));
  connect(previousFaceButton, SIGNAL(clicked()),
          SLOT(previousFace()));
  connect(nextFaceButton, SIGNAL(clicked()),
          SLOT(nextFace()));
  connect(previousInstanceButton, SIGNAL(clicked()),
          SLOT(previousInstance()));
  connect(nextInstanceButton, SIGNAL(clicked()),
          SLOT(nextInstance()));
}


void
MainGUI::createActions()
{
  loadFontsAct = new QAction(tr("&Load Fonts"), this);
  loadFontsAct->setShortcuts(QKeySequence::Open);
  connect(loadFontsAct, SIGNAL(triggered()), SLOT(loadFonts()));

  closeFontAct = new QAction(tr("&Close Font"), this);
  closeFontAct->setShortcuts(QKeySequence::Close);
  connect(closeFontAct, SIGNAL(triggered()), SLOT(closeFont()));

  exitAct = new QAction(tr("E&xit"), this);
  exitAct->setShortcuts(QKeySequence::Quit);
  connect(exitAct, SIGNAL(triggered()), SLOT(close()));

  aboutAct = new QAction(tr("&About"), this);
  connect(aboutAct, SIGNAL(triggered()), SLOT(about()));

  aboutQtAct = new QAction(tr("About &Qt"), this);
  connect(aboutQtAct, SIGNAL(triggered()), SLOT(aboutQt()));
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
  // starting value 0 only works with FreeType 2.6.4 or newer
  faceCounter = 1;

  // set up mappings between property values and combo box indices
  hintingModesTrueTypeHash[TT_INTERPRETER_VERSION_35] = HintingMode_TrueType_v35;
  hintingModesTrueTypeHash[TT_INTERPRETER_VERSION_38] = HintingMode_TrueType_v38;
  // TT_INTERPRETER_VERSION_40, not yet implemented
  hintingModesTrueTypeHash[40] = HintingMode_TrueType_v40;

  hintingModesCFFHash[FT_CFF_HINTING_FREETYPE] = HintingMode_CFF_FreeType;
  hintingModesCFFHash[FT_CFF_HINTING_ADOBE] = HintingMode_CFF_Adobe;

  lcdFilterHash[FT_LCD_FILTER_DEFAULT] = LCDFilter_Default;
  lcdFilterHash[FT_LCD_FILTER_LIGHT] = LCDFilter_Light;
  lcdFilterHash[FT_LCD_FILTER_NONE] = LCDFilter_None;
  lcdFilterHash[FT_LCD_FILTER_LEGACY] = LCDFilter_Legacy;

  // make copies and remove existing elements...
  QHash<int, int> hmTTHash = hintingModesTrueTypeHash;
  if (hmTTHash.contains(engine->ttInterpreterVersionDefault))
    hmTTHash.remove(engine->ttInterpreterVersionDefault);
  if (hmTTHash.contains(engine->ttInterpreterVersionOther))
    hmTTHash.remove(engine->ttInterpreterVersionOther);
  if (hmTTHash.contains(engine->ttInterpreterVersionOther1))
    hmTTHash.remove(engine->ttInterpreterVersionOther1);

  QHash<int, int> hmCFFHash = hintingModesCFFHash;
  if (hmCFFHash.contains(engine->cffHintingEngineDefault))
    hmCFFHash.remove(engine->cffHintingEngineDefault);
  if (hmCFFHash.contains(engine->cffHintingEngineOther))
    hmCFFHash.remove(engine->cffHintingEngineOther);

  // ... to construct a list of always disabled hinting mode combo box items
  hintingModesAlwaysDisabled = hmTTHash.values();
  hintingModesAlwaysDisabled += hmCFFHash.values();

  for (int i = 0; i < hintingModesAlwaysDisabled.size(); i++)
    hintingModeComboBoxx->setItemEnabled(hintingModesAlwaysDisabled[i],
                                         false);

  currentFontIndex = -1;
  currentFaceIndex = -1;
  currentInstanceIndex = -1;

  hintingCheckBox->setChecked(true);

  hintingModeComboBoxx->setCurrentIndex(HintingMode_TrueType_v35);
  antiAliasingComboBoxx->setCurrentIndex(AntiAliasing_LCD);
  lcdFilterComboBox->setCurrentIndex(LCDFilter_Light);

  horizontalHintingCheckBox->setChecked(true);
  verticalHintingCheckBox->setChecked(true);
  blueZoneHintingCheckBox->setChecked(true);

  showBitmapCheckBox->setChecked(true);
  showOutlinesCheckBox->setChecked(true);

  gammaSlider->setValue(18); // 1.8
  sizeDoubleSpinBox->setValue(20);
  dpiSpinBox->setValue(96);
  zoomSpinBox->setValue(100);

  checkHinting();
  checkHintingMode();
  checkAutoHinting();
  checkAntiAliasing();
  checkLcdFilter();
  checkShowPoints();
  checkUnits();
  checkCurrentFontIndex();
  checkCurrentFaceIndex();
  checkCurrentInstanceIndex();
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

  MainGUI gui;
  Engine engine(&gui);

  gui.update(&engine);
  gui.setDefaults();

  gui.show();

  return app.exec();
}


// end of ftinspect.cpp
