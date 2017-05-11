// ftinspect.cpp

// Copyright (C) 2016 by Werner Lemberg.

#include "ftinspect.h"

#include <stdint.h>
#include <cmath>
#include <limits>
#include <stdexcept>

#define VERSION "X.Y.Z"


/////////////////////////////////////////////////////////////////////////////
//
// FaceID
//
/////////////////////////////////////////////////////////////////////////////

FaceID::FaceID()
: fontIndex(-1),
  faceIndex(-1),
  namedInstanceIndex(-1)
{
  // empty
}


FaceID::FaceID(int fontIdx,
               int faceIdx,
               int namedInstanceIdx)
: fontIndex(fontIdx),
  faceIndex(faceIdx),
  namedInstanceIndex(namedInstanceIdx)
{
  // empty
}


bool
FaceID::operator<(const FaceID& other) const
{
  bool ret = false;

  if (fontIndex < other.fontIndex)
    ret = true;
  else if (fontIndex == other.fontIndex)
  {
    if (faceIndex < other.faceIndex)
      ret = true;
    else if (faceIndex == other.faceIndex)
    {
      if (namedInstanceIndex < other.namedInstanceIndex)
        ret = true;
    }
  }

  return ret;
}


// The face requester is a function provided by the client application to
// the cache manager to translate an `abstract' face ID into a real
// `FT_Face' object.
//
// We use a map: `faceID' is the value, and its associated key gives the
// font, face, and named instance indices.  Getting a key from a value is
// slow, but this must be done only once, since `faceRequester' is only
// called if the font is not yet in the cache.

FT_Error
faceRequester(FTC_FaceID ftcFaceID,
              FT_Library library,
              FT_Pointer requestData,
              FT_Face* faceP)
{
  MainGUI* gui = static_cast<MainGUI*>(requestData);

  // `ftcFaceID' is actually an integer
  // -> first convert pointer to same-width integer, then discard superfluous
  //    bits (e.g., on x86_64 where pointers are wider than int)
  int val = static_cast<int>(reinterpret_cast<intptr_t>(ftcFaceID));
  // make sure this does not cause information loss
  Q_ASSERT_X(sizeof(void*) >= sizeof(int),
             "faceRequester",
             "Pointer size must be at least the size of int"
             " in order to treat FTC_FaceID correctly");

  const FaceID& faceID = gui->engine->faceIDMap.key(val);

  // this is the only place where we have to check the validity of the font
  // index; note that the validity of both the face and named instance index
  // is checked by FreeType itself
  if (faceID.fontIndex < 0
      || faceID.fontIndex >= gui->fontList.size())
    return FT_Err_Invalid_Argument;

  QString& font = gui->fontList[faceID.fontIndex];
  int faceIndex = faceID.faceIndex;

  if (faceID.namedInstanceIndex > 0)
    faceIndex += faceID.namedInstanceIndex << 16;

  return FT_New_Face(library,
                     qPrintable(font),
                     faceIndex,
                     faceP);
}


/////////////////////////////////////////////////////////////////////////////
//
// Engine
//
/////////////////////////////////////////////////////////////////////////////

Engine::Engine(MainGUI* g)
{
  gui = g;
  ftSize = NULL;
  // we reserve value 0 for the `invalid face ID'
  faceCounter = 1;

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
    int engines[] =
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
    int interpreters[] =
    {
      TT_INTERPRETER_VERSION_35,
      TT_INTERPRETER_VERSION_38,
      TT_INTERPRETER_VERSION_40
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
}


Engine::~Engine()
{
  FTC_Manager_Done(cacheManager);
  FT_Done_FreeType(library);
}


int
Engine::numberOfFaces(int fontIndex)
{
  FT_Face face;
  int numFaces = -1;

  // search triplet (fontIndex, 0, 0)
  FTC_FaceID ftcFaceID = reinterpret_cast<void*>
                           (faceIDMap.value(FaceID(fontIndex,
                                                   0,
                                                   0)));
  if (ftcFaceID)
  {
    // found
    if (!FTC_Manager_LookupFace(cacheManager, ftcFaceID, &face))
      numFaces = face->num_faces;
  }
  else
  {
    // not found; try to load triplet (fontIndex, 0, 0)
    ftcFaceID = reinterpret_cast<void*>(faceCounter);
    faceIDMap.insert(FaceID(fontIndex, 0, 0),
                     faceCounter++);

    if (!FTC_Manager_LookupFace(cacheManager, ftcFaceID, &face))
      numFaces = face->num_faces;
    else
    {
      faceIDMap.remove(FaceID(fontIndex, 0, 0));
      faceCounter--;
    }
  }

  return numFaces;
}


int
Engine::numberOfNamedInstances(int fontIndex,
                               int faceIndex)
{
  FT_Face face;
  // we return `n' named instances plus one;
  // instance index 0 represents a face without a named instance selected
  int numNamedInstances = -1;

  // search triplet (fontIndex, faceIndex, 0)
  FTC_FaceID ftcFaceID = reinterpret_cast<void*>
                           (faceIDMap.value(FaceID(fontIndex,
                                                   faceIndex,
                                                   0)));
  if (ftcFaceID)
  {
    // found
    if (!FTC_Manager_LookupFace(cacheManager, ftcFaceID, &face))
      numNamedInstances = static_cast<int>((face->style_flags >> 16) + 1);
  }
  else
  {
    // not found; try to load triplet (fontIndex, faceIndex, 0)
    ftcFaceID = reinterpret_cast<void*>(faceCounter);
    faceIDMap.insert(FaceID(fontIndex, faceIndex, 0),
                     faceCounter++);

    if (!FTC_Manager_LookupFace(cacheManager, ftcFaceID, &face))
      numNamedInstances = static_cast<int>((face->style_flags >> 16) + 1);
    else
    {
      faceIDMap.remove(FaceID(fontIndex, faceIndex, 0));
      faceCounter--;
    }
  }

  return numNamedInstances;
}


int
Engine::loadFont(int fontIndex,
                 int faceIndex,
                 int namedInstanceIndex)
{
  int numGlyphs = -1;
  fontType = FontType_Other;

  update();

  // search triplet (fontIndex, faceIndex, namedInstanceIndex)
  scaler.face_id = reinterpret_cast<void*>
                     (faceIDMap.value(FaceID(fontIndex,
                                             faceIndex,
                                             namedInstanceIndex)));
  if (scaler.face_id)
  {
    // found
    if (!FTC_Manager_LookupSize(cacheManager, &scaler, &ftSize))
      numGlyphs = ftSize->face->num_glyphs;
  }
  else
  {
    // not found; try to load triplet
    // (fontIndex, faceIndex, namedInstanceIndex)
    scaler.face_id = reinterpret_cast<void*>(faceCounter);
    faceIDMap.insert(FaceID(fontIndex,
                            faceIndex,
                            namedInstanceIndex),
                     faceCounter++);

    if (!FTC_Manager_LookupSize(cacheManager, &scaler, &ftSize))
      numGlyphs = ftSize->face->num_glyphs;
    else
    {
      faceIDMap.remove(FaceID(fontIndex,
                              faceIndex,
                              namedInstanceIndex));
      faceCounter--;
    }
  }

  if (numGlyphs < 0)
  {
    ftSize = NULL;
    curFamilyName = QString();
    curStyleName = QString();
  }
  else
  {
    curFamilyName = QString(ftSize->face->family_name);
    curStyleName = QString(ftSize->face->style_name);

    FT_Module module = &ftSize->face->driver->root;
    const char* moduleName = module->clazz->module_name;

    // XXX cover all available modules
    if (!strcmp(moduleName, "cff"))
      fontType = FontType_CFF;
    else if (!strcmp(moduleName, "truetype"))
      fontType = FontType_TrueType;
  }

  return numGlyphs;
}


void
Engine::removeFont(int fontIndex)
{
  // we iterate over all triplets that contain the given font index
  // and remove them
  QMap<FaceID, int>::iterator iter
    = faceIDMap.lowerBound(FaceID(fontIndex, 0, 0));

  for (;;)
  {
    if (iter == faceIDMap.end())
      break;

    FaceID faceID = iter.key();
    if (faceID.fontIndex != fontIndex)
      break;

    FTC_FaceID ftcFaceID = reinterpret_cast<void*>(iter.value());
    FTC_Manager_RemoveFaceID(cacheManager, ftcFaceID);

    iter = faceIDMap.erase(iter);
  }
}


const QString&
Engine::currentFamilyName()
{
  return curFamilyName;
}


const QString&
Engine::currentStyleName()
{
  return curStyleName;
}


QString
Engine::glyphName(int index)
{
  QString name;

  if (index < 0)
    throw std::runtime_error("Invalid glyph index");

  if (ftSize && FT_HAS_GLYPH_NAMES(ftSize->face))
  {
    char buffer[256];
    if (!FT_Get_Glyph_Name(ftSize->face,
                           static_cast<unsigned int>(index),
                           buffer,
                           sizeof(buffer)))
      name = QString(buffer);
  }

  return name;
}


FT_Outline*
Engine::loadOutline(int glyphIndex)
{
  update();

  if (glyphIndex < 0)
    throw std::runtime_error("Invalid glyph index");

  FT_Glyph glyph;

  // XXX handle bitmap fonts

  // the `scaler' object is set up by the
  // `update' and `loadFont' methods
  if (FTC_ImageCache_LookupScaler(imageCache,
                                  &scaler,
                                  loadFlags | FT_LOAD_NO_BITMAP,
                                  static_cast<unsigned int>(glyphIndex),
                                  &glyph,
                                  NULL))
  {
    // XXX error handling?
    return NULL;
  }

  if (glyph->format != FT_GLYPH_FORMAT_OUTLINE)
    return NULL;

  FT_OutlineGlyph outlineGlyph = reinterpret_cast<FT_OutlineGlyph>(glyph);

  return &outlineGlyph->outline;
}


void
Engine::setCFFHintingMode(int mode)
{
  int index = gui->hintingModesCFFHash.key(mode);

  FT_Error error = FT_Property_Set(library,
                                   "cff",
                                   "hinting-engine",
                                   &index);
  if (!error)
  {
    // reset the cache
    FTC_Manager_Reset(cacheManager);
  }
}


void
Engine::setTTInterpreterVersion(int mode)
{
  int index = gui->hintingModesTrueTypeHash.key(mode);

  FT_Error error = FT_Property_Set(library,
                                   "truetype",
                                   "interpreter-version",
                                   &index);
  if (!error)
  {
    // reset the cache
    FTC_Manager_Reset(cacheManager);
  }
}


void
Engine::update()
{
  // Spinbox value cannot become negative
  dpi = static_cast<unsigned int>(gui->dpiSpinBox->value());

  if (gui->unitsComboBox->currentIndex() == MainGUI::Units_px)
  {
    pixelSize = gui->sizeDoubleSpinBox->value();
    pointSize = pixelSize * 72.0 / dpi;
  }
  else
  {
    pointSize = gui->sizeDoubleSpinBox->value();
    pixelSize = pointSize * dpi / 72.0;
  }

  doHinting = gui->hintingCheckBox->isChecked();

  doAutoHinting = gui->autoHintingCheckBox->isChecked();
  doHorizontalHinting = gui->horizontalHintingCheckBox->isChecked();
  doVerticalHinting = gui->verticalHintingCheckBox->isChecked();
  doBlueZoneHinting = gui->blueZoneHintingCheckBox->isChecked();
  showSegments = gui->segmentDrawingCheckBox->isChecked();
  doWarping = gui->warpingCheckBox->isChecked();

  gamma = gui->gammaSlider->value();

  loadFlags = FT_LOAD_DEFAULT;
  if (doAutoHinting)
    loadFlags |= FT_LOAD_FORCE_AUTOHINT;
  loadFlags |= FT_LOAD_NO_BITMAP; // XXX handle bitmap fonts also

  int index = gui->antiAliasingComboBoxx->currentIndex();

  if (doHinting)
  {
    unsigned long target;

    if (index == MainGUI::AntiAliasing_None)
      target = FT_LOAD_TARGET_MONO;
    else
    {
      switch (index)
      {
      case MainGUI::AntiAliasing_Light:
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
    scaler.width = static_cast<unsigned int>(pixelSize * 64.0);
    scaler.height = static_cast<unsigned int>(pixelSize * 64.0);
    scaler.x_res = 0;
    scaler.y_res = 0;
  }
  else
  {
    scaler.width = static_cast<unsigned int>(pointSize * 64.0);
    scaler.height = static_cast<unsigned int>(pointSize * 64.0);
    scaler.x_res = dpi;
    scaler.y_res = dpi;
  }
}


/////////////////////////////////////////////////////////////////////////////
//
// Grid
//
/////////////////////////////////////////////////////////////////////////////

Grid::Grid(const QPen& gridP,
           const QPen& axisP)
: gridPen(gridP),
  axisPen(axisP)
{
 // empty
}


QRectF
Grid::boundingRect() const
{
  // XXX fix size

  // no need to take care of pen width
  return QRectF(-100, -100,
                200, 200);
}


// XXX call this in a `myQDraphicsView::drawBackground' derived method
//     to always fill the complete viewport

void
Grid::paint(QPainter* painter,
            const QStyleOptionGraphicsItem* option,
            QWidget*)
{
  const qreal lod = option->levelOfDetailFromTransform(
                              painter->worldTransform());

  painter->setPen(gridPen);

  // don't mark pixel center with a cross if magnification is too small
  if (lod > 20)
  {
    int halfLength = 1;

    // cf. QSpinBoxx
    if (lod > 640)
      halfLength = 6;
    else if (lod > 320)
      halfLength = 5;
    else if (lod > 160)
      halfLength = 4;
    else if (lod > 80)
      halfLength = 3;
    else if (lod > 40)
      halfLength = 2;

    for (qreal x = -100; x < 100; x++)
      for (qreal y = -100; y < 100; y++)
      {
        painter->drawLine(QLineF(x + 0.5, y + 0.5 - halfLength / lod,
                                 x + 0.5, y + 0.5 + halfLength / lod));
        painter->drawLine(QLineF(x + 0.5 - halfLength / lod, y + 0.5,
                                 x + 0.5 + halfLength / lod, y + 0.5));
      }
  }

  // don't draw grid if magnification is too small
  if (lod >= 5)
  {
    // XXX fix size
    for (int x = -100; x <= 100; x++)
      painter->drawLine(x, -100,
                        x, 100);
    for (int y = -100; y <= 100; y++)
      painter->drawLine(-100, y,
                        100, y);
  }

  painter->setPen(axisPen);

  painter->drawLine(0, -100,
                    0, 100);
  painter->drawLine(-100, 0,
                    100, 0);
}


/////////////////////////////////////////////////////////////////////////////
//
// GlyphOutline
//
/////////////////////////////////////////////////////////////////////////////

extern "C" {

// vertical font coordinates are bottom-up,
// while Qt uses top-down

static int
moveTo(const FT_Vector* to,
       void* user)
{
  QPainterPath* path = static_cast<QPainterPath*>(user);

  path->moveTo(qreal(to->x) / 64,
               -qreal(to->y) / 64);

  return 0;
}


static int
lineTo(const FT_Vector* to,
       void* user)
{
  QPainterPath* path = static_cast<QPainterPath*>(user);

  path->lineTo(qreal(to->x) / 64,
               -qreal(to->y) / 64);

  return 0;
}


static int
conicTo(const FT_Vector* control,
        const FT_Vector* to,
        void* user)
{
  QPainterPath* path = static_cast<QPainterPath*>(user);

  path->quadTo(qreal(control->x) / 64,
               -qreal(control->y) / 64,
               qreal(to->x) / 64,
               -qreal(to->y) / 64);

  return 0;
}


static int
cubicTo(const FT_Vector* control1,
        const FT_Vector* control2,
        const FT_Vector* to,
        void* user)
{
  QPainterPath* path = static_cast<QPainterPath*>(user);

  path->cubicTo(qreal(control1->x) / 64,
                -qreal(control1->y) / 64,
                qreal(control2->x) / 64,
                -qreal(control2->y) / 64,
                qreal(to->x) / 64,
                -qreal(to->y) / 64);

  return 0;
}


static FT_Outline_Funcs outlineFuncs =
{
  moveTo,
  lineTo,
  conicTo,
  cubicTo,
  0, // no shift
  0  // no delta
};

} // extern "C"


GlyphOutline::GlyphOutline(const QPen& outlineP,
                           FT_Outline* outln)
: outlinePen(outlineP),
  outline(outln)
{
  FT_BBox cbox;

  qreal halfPenWidth = outlinePen.widthF();

  FT_Outline_Get_CBox(outline, &cbox);

  bRect.setCoords(qreal(cbox.xMin) / 64 - halfPenWidth,
                  -qreal(cbox.yMax) / 64 - halfPenWidth,
                  qreal(cbox.xMax) / 64 + halfPenWidth,
                  -qreal(cbox.yMin) / 64 + halfPenWidth);
}


QRectF
GlyphOutline::boundingRect() const
{
  return bRect;
}


void
GlyphOutline::paint(QPainter* painter,
                    const QStyleOptionGraphicsItem*,
                    QWidget*)
{
  painter->setPen(outlinePen);

  QPainterPath path;
  FT_Outline_Decompose(outline, &outlineFuncs, &path);

  painter->drawPath(path);
}


/////////////////////////////////////////////////////////////////////////////
//
// GlyphPoints
//
/////////////////////////////////////////////////////////////////////////////

GlyphPoints::GlyphPoints(const QPen& onP,
                         const QPen& offP,
                         FT_Outline* outln)
: onPen(onP),
  offPen(offP),
  outline(outln)
{
  FT_BBox cbox;

  qreal halfPenWidth = qMax(onPen.widthF(), offPen.widthF()) / 2;

  FT_Outline_Get_CBox(outline, &cbox);

  bRect.setCoords(qreal(cbox.xMin) / 64 - halfPenWidth,
                  -qreal(cbox.yMax) / 64 - halfPenWidth,
                  qreal(cbox.xMax) / 64 + halfPenWidth,
                  -qreal(cbox.yMin) / 64 + halfPenWidth);
}


QRectF
GlyphPoints::boundingRect() const
{
  return bRect;
}


void
GlyphPoints::paint(QPainter* painter,
                   const QStyleOptionGraphicsItem* option,
                   QWidget*)
{
  const qreal lod = option->levelOfDetailFromTransform(
                              painter->worldTransform());

  // don't draw points if magnification is too small
  if (lod >= 5)
  {
    // we want the same dot size regardless of the scaling;
    // for good optical results, the pen widths should be uneven integers

    // interestingly, using `drawPoint' doesn't work as expected:
    // the larger the zoom, the more horizontally stretched the dot appears
#if 0
    qreal origOnPenWidth = onPen.widthF();
    qreal origOffPenWidth = offPen.widthF();

    onPen.setWidthF(origOnPenWidth / lod);
    offPen.setWidthF(origOffPenWidth / lod);

    for (int i = 0; i < outline->n_points; i++)
    {
      if (outline->tags[i] & FT_CURVE_TAG_ON)
        painter->setPen(onPen);
      else
        painter->setPen(offPen);

      painter->drawPoint(QPointF(qreal(outline->points[i].x) / 64,
                                 -qreal(outline->points[i].y) / 64));
    }

    onPen.setWidthF(origOnPenWidth);
    offPen.setWidthF(origOffPenWidth);
#else
    QBrush onBrush(onPen.color());
    QBrush offBrush(offPen.color());

    painter->setPen(Qt::NoPen);

    qreal onRadius = onPen.widthF() / lod;
    qreal offRadius = offPen.widthF() / lod;

    for (int i = 0; i < outline->n_points; i++)
    {
      if (outline->tags[i] & FT_CURVE_TAG_ON)
      {
        painter->setBrush(onBrush);
        painter->drawEllipse(QPointF(qreal(outline->points[i].x) / 64,
                                     -qreal(outline->points[i].y) / 64),
                             onRadius,
                             onRadius);
      }
      else
      {
        painter->setBrush(offBrush);
        painter->drawEllipse(QPointF(qreal(outline->points[i].x) / 64,
                                     -qreal(outline->points[i].y) / 64),
                             offRadius,
                             offRadius);
      }
    }
#endif
  }
}


/////////////////////////////////////////////////////////////////////////////
//
// GlyphPointNumbers
//
/////////////////////////////////////////////////////////////////////////////

GlyphPointNumbers::GlyphPointNumbers(const QPen& onP,
                                     const QPen& offP,
                                     FT_Outline* outln)
: onPen(onP),
  offPen(offP),
  outline(outln)
{
  FT_BBox cbox;

  FT_Outline_Get_CBox(outline, &cbox);

  // XXX fix bRect size
  bRect.setCoords(qreal(cbox.xMin) / 64,
                  -qreal(cbox.yMax) / 64,
                  qreal(cbox.xMax) / 64,
                  -qreal(cbox.yMin) / 64);
}


QRectF
GlyphPointNumbers::boundingRect() const
{
  return bRect;
}


void
GlyphPointNumbers::paint(QPainter* painter,
                         const QStyleOptionGraphicsItem* option,
                         QWidget*)
{
  const qreal lod = option->levelOfDetailFromTransform(
                              painter->worldTransform());

  // don't draw point numbers if magnification is too small
  if (lod >= 10)
  {
    QFont font = painter->font();

    // the following doesn't work correctly with scaling;
    // it seems that Qt doesn't allow arbitrarily small font sizes
    // that get magnified later on
#if 0
    // we want the same text size regardless of the scaling
    font.setPointSizeF(font.pointSizeF() / lod);
    painter->setFont(font);
#else
    font.setPointSizeF(font.pointSizeF() * 3 / 4);
    painter->setFont(font);

    QBrush onBrush(onPen.color());
    QBrush offBrush(offPen.color());

    painter->scale(1 / lod, 1 / lod);
#endif

    FT_Vector* points = outline->points;
    FT_Short* contours = outline->contours;
    char* tags = outline->tags;

    QVector2D octants[8] = { QVector2D(1, 0),
                             QVector2D(0.707f, -0.707f),
                             QVector2D(0, -1),
                             QVector2D(-0.707f, -0.707f),
                             QVector2D(-1, 0),
                             QVector2D(-0.707f, 0.707f),
                             QVector2D(0, 1),
                             QVector2D(0.707f, 0.707f) };


    short ptIdx = 0;
    for (int contIdx = 0; contIdx < outline->n_contours; contIdx++ )
    {
      for (;;)
      {
        short prevIdx, nextIdx;

        // find previous and next point in outline
        if (contIdx == 0)
        {
          if (contours[contIdx] == 0)
          {
            prevIdx = 0;
            nextIdx = 0;
          }
          else
          {
            prevIdx = ptIdx > 0 ? ptIdx - 1
                                : contours[contIdx];
            nextIdx = ptIdx < contours[contIdx] ? ptIdx + 1
                                                : 0;
          }
        }
        else
        {
          prevIdx = ptIdx > (contours[contIdx - 1] + 1) ? ptIdx - 1
                                                        : contours[contIdx];
          nextIdx = ptIdx < contours[contIdx] ? ptIdx + 1
                                              : contours[contIdx - 1] + 1;
        }

        // get vectors to previous and next point and normalize them;
        QVector2D in(static_cast<float>(points[prevIdx].x
                                        - points[ptIdx].x) / 64,
                     -static_cast<float>(points[prevIdx].y
                                         - points[ptIdx].y) / 64);
        QVector2D out(static_cast<float>(points[nextIdx].x
                                         - points[ptIdx].x) / 64,
                      -static_cast<float>(points[nextIdx].y
                                          - points[ptIdx].y) / 64);

        in = in.normalized();
        out = out.normalized();

        QVector2D middle = in + out;
        // check whether vector is very small, using a threshold of 1/8px
        if (qAbs(middle.x()) < 1.0f / 8
            && qAbs(middle.y()) < 1.0f / 8)
        {
          // in case of vectors in almost exactly opposite directions,
          // use a vector orthogonal to them
          middle.setX(out.y());
          middle.setY(-out.x());

          if (qAbs(middle.x()) < 1.0f / 8
              && qAbs(middle.y()) < 1.0f / 8)
          {
            // use direction based on point index for the offset
            // if we still don't have a good value
            middle = octants[ptIdx % 8];
          }
        }

        // normalize `middle' vector (which is never zero),
        // then multiply by 8 to get some distance between
        // the point and the number
        middle = middle.normalized() * 8;

        // we now position the point number in the opposite
        // direction of the `middle' vector,
        QString number = QString::number(ptIdx);

#if 0
        // this fails, see comment above
        int size = 10000;
        qreal x = qreal(points[ptIdx].x) / 64 - middle.x() / lod;
        qreal y = -qreal(points[ptIdx].y) / 64 - middle.y() / lod;
        QPointF corner(x, y);
        int flags = middle.x() > 0 ? Qt::AlignRight
                                   : Qt::AlignLeft;
        if (flags == Qt::AlignRight)
          corner.rx() -= size;
        QRectF posRect(corner, QSizeF(size, size));

        if (tags[ptIdx] & FT_CURVE_TAG_ON)
          painter->setPen(onPen);
        else
          painter->setPen(offPen);

        painter->drawText(posRect, flags, number);
#else
        // convert text string to a path object
        QPainterPath path;
        path.addText(QPointF(0, 0), font, number);
        QRectF ctrlPtRect = path.controlPointRect();

        qreal x = static_cast<qreal>(points[ptIdx].x) / 64 * lod
                  - static_cast<qreal>(middle.x());
        qreal y = -static_cast<qreal>(points[ptIdx].y) / 64 * lod
                  - static_cast<qreal>(middle.y());

        qreal heuristicOffset = 2;
        if (middle.x() > 0)
          path.translate(x - ctrlPtRect.width() - heuristicOffset,
                         y + ctrlPtRect.height() / 2);
        else
          path.translate(x,
                         y + ctrlPtRect.height() / 2);

        painter->fillPath(path,
                          tags[ptIdx] & FT_CURVE_TAG_ON ? onBrush
                                                        : offBrush);
#endif

        ptIdx++;
        if (ptIdx > contours[contIdx])
          break;
      }
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
//
// GlyphBitmap
//
/////////////////////////////////////////////////////////////////////////////

GlyphBitmap::GlyphBitmap(FT_Outline* outline,
                         FT_Library lib,
                         FT_Pixel_Mode pxlMode,
                         const QVector<QRgb>& monoColorTbl,
                         const QVector<QRgb>& grayColorTbl)
: library(lib),
  pixelMode(pxlMode),
  monoColorTable(monoColorTbl),
  grayColorTable(grayColorTbl)
{
  // make a copy of the outline since we are going to manipulate it
  FT_Outline_New(library,
                 static_cast<unsigned int>(outline->n_points),
                 outline->n_contours,
                 &transformed);
  FT_Outline_Copy(outline, &transformed);

  FT_BBox cbox;
  FT_Outline_Get_CBox(outline, &cbox);

  cbox.xMin &= ~63;
  cbox.yMin &= ~63;
  cbox.xMax = (cbox.xMax + 63) & ~63;
  cbox.yMax = (cbox.yMax + 63) & ~63;

  // we shift the outline to the origin for rendering later on
  FT_Outline_Translate(&transformed, -cbox.xMin, -cbox.yMin);

  bRect.setCoords(cbox.xMin / 64, -cbox.yMax / 64,
                  cbox.xMax / 64, -cbox.yMin / 64);
}


GlyphBitmap::~GlyphBitmap()
{
  FT_Outline_Done(library, &transformed);
}


QRectF
GlyphBitmap::boundingRect() const
{
  return bRect;
}


void
GlyphBitmap::paint(QPainter* painter,
                   const QStyleOptionGraphicsItem* option,
                   QWidget*)
{
  FT_Bitmap bitmap;

  int height = static_cast<int>(ceil(bRect.height()));
  int width = static_cast<int>(ceil(bRect.width()));
  QImage::Format format = QImage::Format_Indexed8;

  // XXX cover LCD and color
  if (pixelMode == FT_PIXEL_MODE_MONO)
    format = QImage::Format_Mono;

  QImage image(QSize(width, height), format);

  if (pixelMode == FT_PIXEL_MODE_MONO)
    image.setColorTable(monoColorTable);
  else
    image.setColorTable(grayColorTable);

  image.fill(0);

  bitmap.rows = static_cast<unsigned int>(height);
  bitmap.width = static_cast<unsigned int>(width);
  bitmap.buffer = image.bits();
  bitmap.pitch = image.bytesPerLine();
  bitmap.pixel_mode = pixelMode;

  FT_Error error = FT_Outline_Get_Bitmap(library,
                                         &transformed,
                                         &bitmap);
  if (error)
  {
    // XXX error handling
    return;
  }

  // `drawImage' doesn't work as expected:
  // the larger the zoom, the more the pixel rectangle positions
  // deviate from the grid lines
#if 0
  painter->drawImage(QPoint(bRect.left(), bRect.top()),
                     image.convertToFormat(
                       QImage::Format_ARGB32_Premultiplied));
#else
  const qreal lod = option->levelOfDetailFromTransform(
                              painter->worldTransform());

  painter->setPen(Qt::NoPen);

  for (int x = 0; x < image.width(); x++)
    for (int y = 0; y < image.height(); y++)
    {
      // be careful not to lose the alpha channel
      QRgb p = image.pixel(x, y);
      painter->fillRect(QRectF(x + bRect.left() - 1 / lod / 2,
                               y + bRect.top() - 1 / lod / 2,
                               1 + 1 / lod,
                               1 + 1 / lod),
                        QColor(qRed(p),
                               qGreen(p),
                               qBlue(p),
                               qAlpha(p)));
    }
#endif
}


/////////////////////////////////////////////////////////////////////////////
//
// MainGUI
//
/////////////////////////////////////////////////////////////////////////////

MainGUI::MainGUI()
{
  engine = NULL;

  fontWatcher = new QFileSystemWatcher;
  // if the current input file is invalid we retry once a second to load it
  timer = new QTimer;
  timer->setInterval(1000);

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
MainGUI::aboutQt()
{
  QApplication::aboutQt();
}


void
MainGUI::loadFonts()
{
  int oldSize = fontList.size();

  QStringList files = QFileDialog::getOpenFileNames(
                        this,
                        tr("Load one or more fonts"),
                        QDir::homePath(),
                        "",
                        NULL,
                        QFileDialog::ReadOnly);

  // XXX sort data, uniquify elements
  fontList.append(files);

  // if we have new fonts, set the current index to the first new one
  if (oldSize < fontList.size())
    currentFontIndex = oldSize;

  showFont();
}


void
MainGUI::closeFont()
{
  if (currentFontIndex < fontList.size())
  {
    engine->removeFont(currentFontIndex);
    fontWatcher->removePath(fontList[currentFontIndex]);
    fontList.removeAt(currentFontIndex);
  }

  // show next font after deletion, i.e., retain index if possible
  if (fontList.size())
  {
    if (currentFontIndex >= fontList.size())
      currentFontIndex = fontList.size() - 1;
  }
  else
    currentFontIndex = 0;

  showFont();
}


void
MainGUI::watchCurrentFont()
{
  timer->stop();
  showFont();
}


void
MainGUI::showFont()
{
  // we do lazy computation of FT_Face objects

  if (currentFontIndex < fontList.size())
  {
    QString& font = fontList[currentFontIndex];
    QFileInfo fileInfo(font);
    QString fontName = fileInfo.fileName();

    if (fileInfo.exists())
    {
      // Qt's file watcher doesn't handle symlinks;
      // we thus fall back to polling
      if (fileInfo.isSymLink())
      {
        fontName.prepend("<i>");
        fontName.append("</i>");
        timer->start();
      }
      else
        fontWatcher->addPath(font);
    }
    else
    {
      // On Unix-like systems, the symlink's target gets opened; this
      // implies that deletion of a symlink doesn't make `engine->loadFont'
      // fail since it operates on a file handle pointing to the target.
      // For this reason, we remove the font to enforce a reload.
      engine->removeFont(currentFontIndex);
    }

    fontFilenameLabel->setText(fontName);
  }
  else
    fontFilenameLabel->clear();

  currentNumberOfFaces
    = engine->numberOfFaces(currentFontIndex);
  currentNumberOfNamedInstances
    = engine->numberOfNamedInstances(currentFontIndex,
                                     currentFaceIndex);
  currentNumberOfGlyphs
    = engine->loadFont(currentFontIndex,
                       currentFaceIndex,
                       currentNamedInstanceIndex);

  if (currentNumberOfGlyphs < 0)
  {
    // there might be various reasons why the current
    // (file, face, instance) triplet is invalid or missing;
    // we thus start our timer to periodically test
    // whether the font starts working
    if (currentFontIndex < fontList.size())
      timer->start();
  }

  fontNameLabel->setText(QString("%1 %2")
                         .arg(engine->currentFamilyName())
                         .arg(engine->currentStyleName()));

  checkCurrentFontIndex();
  checkCurrentFaceIndex();
  checkCurrentNamedInstanceIndex();
  checkHinting();
  adjustGlyphIndex(0);

  drawGlyph();
}


void
MainGUI::checkHinting()
{
  if (hintingCheckBox->isChecked())
  {
    if (engine->fontType == Engine::FontType_CFF)
    {
      for (int i = 0; i < hintingModeComboBoxx->count(); i++)
      {
        if (hintingModesCFFHash.key(i, -1) != -1)
          hintingModeComboBoxx->setItemEnabled(i, true);
        else
          hintingModeComboBoxx->setItemEnabled(i, false);
      }

      hintingModeComboBoxx->setCurrentIndex(currentCFFHintingMode);
    }
    else if (engine->fontType == Engine::FontType_TrueType)
    {
      for (int i = 0; i < hintingModeComboBoxx->count(); i++)
      {
        if (hintingModesTrueTypeHash.key(i, -1) != -1)
          hintingModeComboBoxx->setItemEnabled(i, true);
        else
          hintingModeComboBoxx->setItemEnabled(i, false);
      }

      hintingModeComboBoxx->setCurrentIndex(currentTTInterpreterVersion);
    }
    else
    {
      hintingModeLabel->setEnabled(false);
      hintingModeComboBoxx->setEnabled(false);
    }

    for (int i = 0; i < hintingModesAlwaysDisabled.size(); i++)
      hintingModeComboBoxx->setItemEnabled(hintingModesAlwaysDisabled[i],
                                           false);

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

    antiAliasingComboBoxx->setItemEnabled(AntiAliasing_Light, false);
  }

  drawGlyph();
}


void
MainGUI::checkHintingMode()
{
  int index = hintingModeComboBoxx->currentIndex();

  if (engine->fontType == Engine::FontType_CFF)
  {
    engine->setCFFHintingMode(index);
    currentCFFHintingMode = index;
  }
  else if (engine->fontType == Engine::FontType_TrueType)
  {
    engine->setTTInterpreterVersion(index);
    currentTTInterpreterVersion = index;
  }

  // this enforces reloading of the font
  showFont();
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

    antiAliasingComboBoxx->setItemEnabled(AntiAliasing_Light, true);
  }
  else
  {
    if (engine->fontType == Engine::FontType_CFF
        || engine->fontType == Engine::FontType_TrueType)
    {
      hintingModeLabel->setEnabled(true);
      hintingModeComboBoxx->setEnabled(true);
    }

    horizontalHintingCheckBox->setEnabled(false);
    verticalHintingCheckBox->setEnabled(false);
    blueZoneHintingCheckBox->setEnabled(false);
    segmentDrawingCheckBox->setEnabled(false);
    warpingCheckBox->setEnabled(false);

    antiAliasingComboBoxx->setItemEnabled(AntiAliasing_Light, false);

    if (antiAliasingComboBoxx->currentIndex() == AntiAliasing_Light)
      antiAliasingComboBoxx->setCurrentIndex(AntiAliasing_Normal);
  }

  drawGlyph();
}


void
MainGUI::checkAntiAliasing()
{
  int index = antiAliasingComboBoxx->currentIndex();

  if (index == AntiAliasing_None
      || index == AntiAliasing_Normal
      || index == AntiAliasing_Light)
  {
    lcdFilterLabel->setEnabled(false);
    lcdFilterComboBox->setEnabled(false);
  }
  else
  {
    lcdFilterLabel->setEnabled(true);
    lcdFilterComboBox->setEnabled(true);
  }

  drawGlyph();
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
    showPointNumbersCheckBox->setEnabled(true);
  else
    showPointNumbersCheckBox->setEnabled(false);

  drawGlyph();
}


void
MainGUI::checkUnits()
{
  int index = unitsComboBox->currentIndex();

  if (index == Units_px)
  {
    dpiLabel->setEnabled(false);
    dpiSpinBox->setEnabled(false);
    sizeDoubleSpinBox->setSingleStep(1);
    sizeDoubleSpinBox->setValue(qRound(sizeDoubleSpinBox->value()));
  }
  else
  {
    dpiLabel->setEnabled(true);
    dpiSpinBox->setEnabled(true);
    sizeDoubleSpinBox->setSingleStep(0.5);
  }

  drawGlyph();
}


void
MainGUI::adjustGlyphIndex(int delta)
{
  // only adjust current glyph index if we have a valid font
  if (currentNumberOfGlyphs > 0)
  {
    currentGlyphIndex += delta;
    currentGlyphIndex = qBound(0,
                               currentGlyphIndex,
                               currentNumberOfGlyphs - 1);
  }

  QString upperHex = QString::number(currentGlyphIndex, 16).toUpper();
  glyphIndexLabel->setText(QString("%1 (0x%2)")
                                   .arg(currentGlyphIndex)
                                   .arg(upperHex));
  glyphNameLabel->setText(engine->glyphName(currentGlyphIndex));

  drawGlyph();
}


void
MainGUI::checkCurrentFontIndex()
{
  if (fontList.size() < 2)
  {
    previousFontButton->setEnabled(false);
    nextFontButton->setEnabled(false);
  }
  else if (currentFontIndex == 0)
  {
    previousFontButton->setEnabled(false);
    nextFontButton->setEnabled(true);
  }
  else if (currentFontIndex >= fontList.size() - 1)
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
  if (currentNumberOfFaces < 2)
  {
    previousFaceButton->setEnabled(false);
    nextFaceButton->setEnabled(false);
  }
  else if (currentFaceIndex == 0)
  {
    previousFaceButton->setEnabled(false);
    nextFaceButton->setEnabled(true);
  }
  else if (currentFaceIndex >= currentNumberOfFaces - 1)
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
MainGUI::checkCurrentNamedInstanceIndex()
{
  if (currentNumberOfNamedInstances < 2)
  {
    previousNamedInstanceButton->setEnabled(false);
    nextNamedInstanceButton->setEnabled(false);
  }
  else if (currentNamedInstanceIndex == 0)
  {
    previousNamedInstanceButton->setEnabled(false);
    nextNamedInstanceButton->setEnabled(true);
  }
  else if (currentNamedInstanceIndex >= currentNumberOfNamedInstances - 1)
  {
    previousNamedInstanceButton->setEnabled(true);
    nextNamedInstanceButton->setEnabled(false);
  }
  else
  {
    previousNamedInstanceButton->setEnabled(true);
    nextNamedInstanceButton->setEnabled(true);
  }
}


void
MainGUI::previousFont()
{
  if (currentFontIndex > 0)
  {
    currentFontIndex--;
    currentFaceIndex = 0;
    currentNamedInstanceIndex = 0;
    showFont();
  }
}


void
MainGUI::nextFont()
{
  if (currentFontIndex < fontList.size() - 1)
  {
    currentFontIndex++;
    currentFaceIndex = 0;
    currentNamedInstanceIndex = 0;
    showFont();
  }
}


void
MainGUI::previousFace()
{
  if (currentFaceIndex > 0)
  {
    currentFaceIndex--;
    currentNamedInstanceIndex = 0;
    showFont();
  }
}


void
MainGUI::nextFace()
{
  if (currentFaceIndex < currentNumberOfFaces - 1)
  {
    currentFaceIndex++;
    currentNamedInstanceIndex = 0;
    showFont();
  }
}


void
MainGUI::previousNamedInstance()
{
  if (currentNamedInstanceIndex > 0)
  {
    currentNamedInstanceIndex--;
    showFont();
  }
}


void
MainGUI::nextNamedInstance()
{
  if (currentNamedInstanceIndex < currentNumberOfNamedInstances - 1)
  {
    currentNamedInstanceIndex++;
    showFont();
  }
}


void
MainGUI::zoom()
{
  int scale = zoomSpinBox->value();

  QTransform transform;
  transform.scale(scale, scale);

  // we want horizontal and vertical 1px lines displayed with full pixels;
  // we thus have to shift the coordinate system accordingly, using a value
  // that represents 0.5px (i.e., half the 1px line width) after the scaling
  qreal shift = 0.5 / scale;
  transform.translate(shift, shift);

  glyphView->setTransform(transform);
}


void
MainGUI::setGraphicsDefaults()
{
  // color tables (with suitable opacity values) for converting
  // FreeType's pixmaps to something Qt understands
  monoColorTable.append(QColor(Qt::transparent).rgba());
  monoColorTable.append(QColor(Qt::black).rgba());

  for (int i = 0xFF; i >= 0; i--)
    grayColorTable.append(qRgba(i, i, i, 0xFF - i));

  // XXX make this user-configurable

  axisPen.setColor(Qt::black);
  axisPen.setWidth(0);
  blueZonePen.setColor(QColor(64, 64, 255, 64)); // light blue
  blueZonePen.setWidth(0);
  gridPen.setColor(Qt::lightGray);
  gridPen.setWidth(0);
  offPen.setColor(Qt::darkGreen);
  offPen.setWidth(3);
  onPen.setColor(Qt::red);
  onPen.setWidth(3);
  outlinePen.setColor(Qt::red);
  outlinePen.setWidth(0);
  segmentPen.setColor(QColor(64, 255, 128, 64)); // light green
  segmentPen.setWidth(0);
}


void
MainGUI::drawGlyph()
{
  // the call to `engine->loadOutline' updates FreeType's load flags

  if (!engine)
    return;

  if (currentGlyphBitmapItem)
  {
    glyphScene->removeItem(currentGlyphBitmapItem);
    delete currentGlyphBitmapItem;

    currentGlyphBitmapItem = NULL;
  }

  if (currentGlyphOutlineItem)
  {
    glyphScene->removeItem(currentGlyphOutlineItem);
    delete currentGlyphOutlineItem;

    currentGlyphOutlineItem = NULL;
  }

  if (currentGlyphPointsItem)
  {
    glyphScene->removeItem(currentGlyphPointsItem);
    delete currentGlyphPointsItem;

    currentGlyphPointsItem = NULL;
  }

  if (currentGlyphPointNumbersItem)
  {
    glyphScene->removeItem(currentGlyphPointNumbersItem);
    delete currentGlyphPointNumbersItem;

    currentGlyphPointNumbersItem = NULL;
  }

  FT_Outline* outline = engine->loadOutline(currentGlyphIndex);
  if (outline)
  {
    if (showBitmapCheckBox->isChecked())
    {
      // XXX support LCD
      FT_Pixel_Mode pixelMode = FT_PIXEL_MODE_GRAY;
      if (antiAliasingComboBoxx->currentIndex() == AntiAliasing_None)
        pixelMode = FT_PIXEL_MODE_MONO;

      currentGlyphBitmapItem = new GlyphBitmap(outline,
                                               engine->library,
                                               pixelMode,
                                               monoColorTable,
                                               grayColorTable);
      glyphScene->addItem(currentGlyphBitmapItem);
    }

    if (showOutlinesCheckBox->isChecked())
    {
      currentGlyphOutlineItem = new GlyphOutline(outlinePen, outline);
      glyphScene->addItem(currentGlyphOutlineItem);
    }

    if (showPointsCheckBox->isChecked())
    {
      currentGlyphPointsItem = new GlyphPoints(onPen, offPen, outline);
      glyphScene->addItem(currentGlyphPointsItem);

      if (showPointNumbersCheckBox->isChecked())
      {
        currentGlyphPointNumbersItem = new GlyphPointNumbers(onPen,
                                                             offPen,
                                                             outline);
        glyphScene->addItem(currentGlyphPointNumbersItem);
      }
    }
  }

  glyphScene->update();
}


// XXX distances are specified in pixels,
//     making the layout dependent on the output device resolution
void
MainGUI::createLayout()
{
  // left side
  fontFilenameLabel = new QLabel;

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
  antiAliasingComboBoxx->insertItem(AntiAliasing_Light,
                                    tr("Light"));
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
  showPointNumbersCheckBox = new QCheckBox(tr("Show Point Numbers"));
  showOutlinesCheckBox = new QCheckBox(tr("Show Outlines"));

  infoLeftLayout = new QHBoxLayout;
  infoLeftLayout->addWidget(fontFilenameLabel);

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

  pointNumbersLayout = new QHBoxLayout;
  pointNumbersLayout->addSpacing(20); // XXX px
  pointNumbersLayout->addWidget(showPointNumbersCheckBox);

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
  generalTabLayout->addLayout(pointNumbersLayout);
  generalTabLayout->addWidget(showOutlinesCheckBox);

  generalTabWidget = new QWidget;
  generalTabWidget->setLayout(generalTabLayout);

  mmgxTabWidget = new QWidget;

  tabWidget = new QTabWidget;
  tabWidget->addTab(generalTabWidget, tr("General"));
  tabWidget->addTab(mmgxTabWidget, tr("MM/GX"));

  leftLayout = new QVBoxLayout;
  leftLayout->addLayout(infoLeftLayout);
  leftLayout->addWidget(tabWidget);

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
  glyphIndexLabel = new QLabel;
  glyphNameLabel = new QLabel;
  fontNameLabel = new QLabel;

  glyphScene = new QGraphicsScene;
  glyphScene->addItem(new Grid(gridPen, axisPen));

  currentGlyphBitmapItem = NULL;
  currentGlyphOutlineItem = NULL;
  currentGlyphPointsItem = NULL;
  currentGlyphPointNumbersItem = NULL;
  drawGlyph();

  glyphView = new QGraphicsViewx;
  glyphView->setRenderHint(QPainter::Antialiasing, true);
  glyphView->setDragMode(QGraphicsView::ScrollHandDrag);
  glyphView->setOptimizationFlags(QGraphicsView::DontSavePainterState);
  glyphView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
  glyphView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  glyphView->setScene(glyphScene);

  sizeLabel = new QLabel(tr("Size "));
  sizeLabel->setAlignment(Qt::AlignRight);
  sizeDoubleSpinBox = new QDoubleSpinBox;
  sizeDoubleSpinBox->setAlignment(Qt::AlignRight);
  sizeDoubleSpinBox->setDecimals(1);
  sizeDoubleSpinBox->setRange(1, 500);
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

  zoomLabel = new QLabel(tr("Zoom Factor"));
  zoomLabel->setAlignment(Qt::AlignRight);
  zoomSpinBox = new QSpinBoxx;
  zoomSpinBox->setAlignment(Qt::AlignRight);
  zoomSpinBox->setRange(1, 1000 - 1000 % 64);
  zoomSpinBox->setKeyboardTracking(false);
  zoomLabel->setBuddy(zoomSpinBox);

  previousFontButton = new QPushButton(tr("Previous Font"));
  nextFontButton = new QPushButton(tr("Next Font"));
  previousFaceButton = new QPushButton(tr("Previous Face"));
  nextFaceButton = new QPushButton(tr("Next Face"));
  previousNamedInstanceButton = new QPushButton(tr("Previous Named Instance"));
  nextNamedInstanceButton = new QPushButton(tr("Next Named Instance"));

  infoRightLayout = new QGridLayout;
  infoRightLayout->addWidget(glyphIndexLabel, 0, 0);
  infoRightLayout->addWidget(glyphNameLabel, 0, 1);
  infoRightLayout->addWidget(fontNameLabel, 0, 2);

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
  fontLayout->addWidget(nextNamedInstanceButton, 0, 5);
  fontLayout->addWidget(previousNamedInstanceButton, 1, 5);
  fontLayout->setColumnStretch(6, 2);

  rightLayout = new QVBoxLayout;
  rightLayout->addLayout(infoRightLayout);
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
  connect(showBitmapCheckBox, SIGNAL(clicked()),
          SLOT(drawGlyph()));
  connect(showPointsCheckBox, SIGNAL(clicked()),
          SLOT(checkShowPoints()));
  connect(showPointNumbersCheckBox, SIGNAL(clicked()),
          SLOT(drawGlyph()));
  connect(showOutlinesCheckBox, SIGNAL(clicked()),
          SLOT(drawGlyph()));

  connect(sizeDoubleSpinBox, SIGNAL(valueChanged(double)),
          SLOT(drawGlyph()));
  connect(unitsComboBox, SIGNAL(currentIndexChanged(int)),
          SLOT(checkUnits()));
  connect(dpiSpinBox, SIGNAL(valueChanged(int)),
          SLOT(drawGlyph()));

  connect(zoomSpinBox, SIGNAL(valueChanged(int)),
          SLOT(zoom()));

  connect(previousFontButton, SIGNAL(clicked()),
          SLOT(previousFont()));
  connect(nextFontButton, SIGNAL(clicked()),
          SLOT(nextFont()));
  connect(previousFaceButton, SIGNAL(clicked()),
          SLOT(previousFace()));
  connect(nextFaceButton, SIGNAL(clicked()),
          SLOT(nextFace()));
  connect(previousNamedInstanceButton, SIGNAL(clicked()),
          SLOT(previousNamedInstance()));
  connect(nextNamedInstanceButton, SIGNAL(clicked()),
          SLOT(nextNamedInstance()));

  glyphNavigationMapper = new QSignalMapper;
  connect(glyphNavigationMapper, SIGNAL(mapped(int)),
          SLOT(adjustGlyphIndex(int)));

  connect(toStartButtonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toM1000Buttonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toM100Buttonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toM10Buttonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toM1Buttonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toP1Buttonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toP10Buttonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toP100Buttonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toP1000Buttonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toEndButtonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));

  glyphNavigationMapper->setMapping(toStartButtonx, -0x10000);
  glyphNavigationMapper->setMapping(toM1000Buttonx, -1000);
  glyphNavigationMapper->setMapping(toM100Buttonx, -100);
  glyphNavigationMapper->setMapping(toM10Buttonx, -10);
  glyphNavigationMapper->setMapping(toM1Buttonx, -1);
  glyphNavigationMapper->setMapping(toP1Buttonx, 1);
  glyphNavigationMapper->setMapping(toP10Buttonx, 10);
  glyphNavigationMapper->setMapping(toP100Buttonx, 100);
  glyphNavigationMapper->setMapping(toP1000Buttonx, 1000);
  glyphNavigationMapper->setMapping(toEndButtonx, 0x10000);

  connect(fontWatcher, SIGNAL(fileChanged(const QString&)),
          SLOT(watchCurrentFont()));
  connect(timer, SIGNAL(timeout()),
          SLOT(watchCurrentFont()));
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
  // set up mappings between property values and combo box indices
  hintingModesTrueTypeHash[TT_INTERPRETER_VERSION_35] = HintingMode_TrueType_v35;
  hintingModesTrueTypeHash[TT_INTERPRETER_VERSION_38] = HintingMode_TrueType_v38;
  hintingModesTrueTypeHash[TT_INTERPRETER_VERSION_40] = HintingMode_TrueType_v40;

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

  // the next four values always non-negative
  currentFontIndex = 0;
  currentFaceIndex = 0;
  currentNamedInstanceIndex = 0;
  currentGlyphIndex = 0;

  currentCFFHintingMode
    = hintingModesCFFHash[engine->cffHintingEngineDefault];
  currentTTInterpreterVersion
    = hintingModesTrueTypeHash[engine->ttInterpreterVersionDefault];

  hintingCheckBox->setChecked(true);

  antiAliasingComboBoxx->setCurrentIndex(AntiAliasing_Normal);
  lcdFilterComboBox->setCurrentIndex(LCDFilter_Light);

  horizontalHintingCheckBox->setChecked(true);
  verticalHintingCheckBox->setChecked(true);
  blueZoneHintingCheckBox->setChecked(true);

  showBitmapCheckBox->setChecked(true);
  showOutlinesCheckBox->setChecked(true);

  gammaSlider->setValue(18); // 1.8
  sizeDoubleSpinBox->setValue(20);
  dpiSpinBox->setValue(96);
  zoomSpinBox->setValue(20);

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


/////////////////////////////////////////////////////////////////////////////
//
// QGraphicsViewx
//
/////////////////////////////////////////////////////////////////////////////

QGraphicsViewx::QGraphicsViewx()
: lastBottomLeftPointInitialized(false)
{
  // empty
}


void
QGraphicsViewx::scrollContentsBy(int dx,
                                 int dy)
{
  QGraphicsView::scrollContentsBy(dx, dy);
  lastBottomLeftPoint = viewport()->rect().bottomLeft();
}


void
QGraphicsViewx::resizeEvent(QResizeEvent* event)
{
  QGraphicsView::resizeEvent(event);

  // XXX I don't know how to properly initialize this value,
  //     thus the hack with the boolean
  if (!lastBottomLeftPointInitialized)
  {
    lastBottomLeftPoint = viewport()->rect().bottomLeft();
    lastBottomLeftPointInitialized = true;
  }

  QPointF currentBottomLeftPoint = viewport()->rect().bottomLeft();
  int verticalPosition = verticalScrollBar()->value();
  verticalScrollBar()->setValue(static_cast<int>(
                                  verticalPosition
                                  - (currentBottomLeftPoint.y()
                                     - lastBottomLeftPoint.y())));
}


/////////////////////////////////////////////////////////////////////////////
//
// QSpinBoxx
//
/////////////////////////////////////////////////////////////////////////////

// we want to mark the center of a pixel square with a single dot or a small
// cross; starting with a certain magnification we thus only use even values
// so that we can do that symmetrically

int
QSpinBoxx::valueFromText(const QString& text) const
{
  int val = QSpinBox::valueFromText(text);

  if (val > 640)
    val = val - (val % 64);
  else if (val > 320)
    val = val - (val % 32);
  else if (val > 160)
    val = val - (val % 16);
  else if (val > 80)
    val = val - (val % 8);
  else if (val > 40)
    val = val - (val % 4);
  else if (val > 20)
    val = val - (val % 2);

  return val;
}


void
QSpinBoxx::stepBy(int steps)
{
  int val = value();

  if (steps > 0)
  {
    for (int i = 0; i < steps; i++)
    {
      if (val >= 640)
        val = val + 64;
      else if (val >= 320)
        val = val + 32;
      else if (val >= 160)
        val = val + 16;
      else if (val >= 80)
        val = val + 8;
      else if (val >= 40)
        val = val + 4;
      else if (val >= 20)
        val = val + 2;
      else
        val++;
    }
  }
  else if (steps < 0)
  {
    for (int i = 0; i < -steps; i++)
    {
      if (val > 640)
        val = val - 64;
      else if (val > 320)
        val = val - 32;
      else if (val > 160)
        val = val - 16;
      else if (val > 80)
        val = val - 8;
      else if (val > 40)
        val = val - 4;
      else if (val > 20)
        val = val - 2;
      else
        val--;
    }
  }

  setValue(val);
}


/////////////////////////////////////////////////////////////////////////////
//
// QComboBoxx
//
/////////////////////////////////////////////////////////////////////////////

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


/////////////////////////////////////////////////////////////////////////////
//
// QPushButtonx
//
/////////////////////////////////////////////////////////////////////////////

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


/////////////////////////////////////////////////////////////////////////////
//
// main
//
/////////////////////////////////////////////////////////////////////////////

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
