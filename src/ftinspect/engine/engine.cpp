// engine.cpp

// Copyright (C) 2016-2023 by
// Werner Lemberg.


#include "engine.hpp"

#include <stdexcept>
#include <stdint.h>

#include <freetype/ftdriver.h>
#include <freetype/ftlcdfil.h>
#include <freetype/ftmm.h>
#include <freetype/ftmodapi.h>


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


FaceID::FaceID(int fontIndex,
               long faceIndex,
               int namedInstanceIndex)
: fontIndex(fontIndex),
  faceIndex(faceIndex),
  namedInstanceIndex(namedInstanceIndex)
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
// the cache manager to translate an 'abstract' face ID into a real
// `FT_Face' object.
//
// We use a map: `faceID` is the value, and its associated key gives the
// font, face, and named instance indices.  Getting a key from a value is
// slow, but this must be done only once, since `faceRequester` is only
// called if the font is not yet in the cache.

FT_Error
faceRequester(FTC_FaceID ftcFaceID,
              FT_Library library,
              FT_Pointer requestData,
              FT_Face* faceP)
{
  Engine* engine = static_cast<Engine*>(requestData);
  // `ftcFaceID` is actually an integer
  // -> First convert pointer to same-width integer, then discard superfluous
  //    bits (e.g., on x86_64 where pointers are wider than `int`).
  int val = static_cast<int>(reinterpret_cast<intptr_t>(ftcFaceID));
  // Make sure this does not cause information loss.
  Q_ASSERT_X(sizeof(void*) >= sizeof(int),
             "faceRequester",
             "Pointer size must be at least the size of int"
             " in order to treat FTC_FaceID correctly");

  const FaceID& faceID = engine->faceIDMap_.key(val);

  // This is the only place where we have to check the validity of the font
  // index; note that the validity of both the face and named instance index
  // is checked by FreeType itself.
  if (faceID.fontIndex < 0
      || faceID.fontIndex >= engine->numberOfOpenedFonts())
    return FT_Err_Invalid_Argument;

  QString font = engine->fontFileManager_[faceID.fontIndex].filePath();
  long faceIndex = faceID.faceIndex;

  if (faceID.namedInstanceIndex > 0)
    faceIndex += faceID.namedInstanceIndex << 16;

  *faceP = NULL;
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

Engine::Engine()
: fontFileManager_(this)
{
  ftSize_ = NULL;
  ftFallbackFace_ = NULL;
  // We reserve value 0 for the 'invalid face ID'.
  faceCounter_ = 1;

  FT_Error error;

  error = FT_Init_FreeType(&library_);
  if (error)
  {
    // XXX error handling
  }

  error = FTC_Manager_New(library_, 0, 0, 0,
                          faceRequester, this, &cacheManager_);
  if (error)
  {
    // XXX error handling
  }

  error = FTC_SBitCache_New(cacheManager_, &sbitsCache_);
  if (error)
  {
    // XXX error handling
  }

  error = FTC_ImageCache_New(cacheManager_, &imageCache_);
  if (error)
  {
    // XXX error handling
  }

  error = FTC_CMapCache_New(cacheManager_, &cmapCache_);
  if (error)
  {
    // XXX error handling
  }

  queryEngine();
  renderingEngine_
    = std::unique_ptr<RenderingEngine>(new RenderingEngine(this));
}


Engine::~Engine()
{
  FTC_Manager_Done(cacheManager_);
  FT_Done_FreeType(library_);
}


template <class Func>
void
Engine::withFace(FaceID id,
                 Func func)
{
  FT_Face face;
  // Search triplet (fontIndex, faceIndex, namedInstanceIndex).
  auto numId = reinterpret_cast<FTC_FaceID>(faceIDMap_.value(id));
  if (numId)
  {
    // Found.
    if (!FTC_Manager_LookupFace(cacheManager_, numId, &face))
      func(face);
  }
  else if (id.fontIndex >= 0)
  {
    if (faceCounter_ >= INT_MAX) // Prevent overflow.
      return;

    // Not found; try to load triplet
    // (fontIndex, faceIndex, namedInstanceIndex).
    numId = reinterpret_cast<FTC_FaceID>(faceCounter_);
    faceIDMap_.insert(id, faceCounter_++);

    if (!FTC_Manager_LookupFace(cacheManager_, numId, &face))
      func(face);
    else
    {
      faceIDMap_.remove(id);
      faceCounter_--;
    }
  }
}


long
Engine::numberOfFaces(int fontIndex)
{
  long numFaces = -1;

  if (fontIndex < 0)
    return -1;

  // Search triplet (fontIndex, 0, 0).
  withFace(FaceID(fontIndex, 0, 0),
           [&](FT_Face face) { numFaces = face->num_faces; });

  return numFaces;
}


int
Engine::numberOfNamedInstances(int fontIndex,
                               long faceIndex)
{
  // We return `n` named instances plus one;
  // instance index 0 represents a face without a named instance selected.
  int numNamedInstances = -1;
  if (fontIndex < 0)
    return -1;

  withFace(FaceID(fontIndex, faceIndex, 0),
           [&](FT_Face face)
           {
             numNamedInstances
               = static_cast<int>((face->style_flags >> 16) + 1);
           });

  return numNamedInstances;
}


QString
Engine::namedInstanceName(int fontIndex,
                          long faceIndex,
                          int index)
{
  if (fontIndex < 0)
    return {};

  QString name;
  withFace(FaceID(fontIndex, faceIndex, index),
           [&](FT_Face face)
           {
             name = QString("%1 %2")
                      .arg(face->family_name)
                      .arg(face->style_name);
           });
  return name;
}


bool
Engine::currentFontTricky()
{
  if (!ftFallbackFace_)
    return false;
  return FT_IS_TRICKY(ftFallbackFace_);
}


int
Engine::loadFont(int fontIndex,
                 long faceIndex,
                 int namedInstanceIndex)
{
  int numGlyphs = -1;
  fontType_ = FontType_Other;
  palette_ = NULL;

  update();

  curFontIndex_ = fontIndex;
  auto id = FaceID(fontIndex, faceIndex, namedInstanceIndex);

  // Search triplet (fontIndex, faceIndex, namedInstanceIndex).
  scaler_.face_id = reinterpret_cast<FTC_FaceID>(faceIDMap_.value(id));
  if (scaler_.face_id)
  {
    // Found.
    if (!FTC_Manager_LookupFace(cacheManager_, scaler_.face_id,
                               &ftFallbackFace_))
    {
      numGlyphs = ftFallbackFace_->num_glyphs;
      if (FTC_Manager_LookupSize(cacheManager_, &scaler_, &ftSize_))
        ftSize_ = NULL; // Good font, bad size.
    }
    else
    {
      ftFallbackFace_ = NULL;
      ftSize_ = NULL;
    }
  }
  else if (fontIndex >= 0)
  {
    if (faceCounter_ >= INT_MAX) // Prevent overflow.
      return -1;

    // Not found; try to load triplet
    // (fontIndex, faceIndex, namedInstanceIndex).
    scaler_.face_id = reinterpret_cast<FTC_FaceID>(faceCounter_);
    faceIDMap_.insert(id, faceCounter_++);

    if (!FTC_Manager_LookupFace(cacheManager_, scaler_.face_id,
                                &ftFallbackFace_))
    {
      numGlyphs = ftFallbackFace_->num_glyphs;
      if (FTC_Manager_LookupSize(cacheManager_, &scaler_, &ftSize_))
        ftSize_ = NULL; // Good font, bad size.
    }
    else
    {
      faceIDMap_.remove(id);
      faceCounter_--;
      ftFallbackFace_ = NULL;
      ftSize_ = NULL;
    }
  }

  imageType_.face_id = scaler_.face_id;

  if (numGlyphs < 0)
  {
    ftFallbackFace_ = NULL;
    ftSize_ = NULL;
    curFamilyName_ = QString();
    curStyleName_ = QString();

    curCharMaps_.clear();
    curPaletteInfos_.clear();
    curSFNTNames_.clear();
  }
  else
  {
    curFamilyName_ = QString(ftFallbackFace_->family_name);
    curStyleName_ = QString(ftFallbackFace_->style_name);

    const char* moduleName = FT_FACE_DRIVER_NAME(ftFallbackFace_);

    // XXX cover all available modules
    if (!strcmp(moduleName, "cff"))
      fontType_ = FontType_CFF;
    else if (!strcmp(moduleName, "truetype"))
      fontType_ = FontType_TrueType;
    else
      fontType_ = FontType_Other;

    curCharMaps_.clear();
    curCharMaps_.reserve(ftFallbackFace_->num_charmaps);
    for (int i = 0; i < ftFallbackFace_->num_charmaps; i++)
      curCharMaps_.emplace_back(i, ftFallbackFace_->charmaps[i]);

    SFNTName::get(this, curSFNTNames_);
    loadPaletteInfos();
    curMMGXState_ = MMGXAxisInfo::get(this, curMMGXAxes_);
  }

  curNumGlyphs_ = numGlyphs;
  return numGlyphs;
}


void
Engine::reloadFont()
{
  update();
  palette_ = NULL;
  if (!scaler_.face_id)
    return;
  imageType_.face_id = scaler_.face_id;

  if (FTC_Manager_LookupFace(cacheManager_,
                             scaler_.face_id,
                             &ftFallbackFace_))
  {
    ftFallbackFace_ = NULL;
    ftSize_ = NULL;
    return;
  }
  if (FTC_Manager_LookupSize(cacheManager_, &scaler_, &ftSize_))
    ftSize_ = NULL; // Good font, bad size.
}


void
Engine::loadPalette()
{
  palette_ = NULL;
  if (paletteData_.num_palettes == 0
      || paletteIndex_ < 0
      || paletteData_.num_palettes <= paletteIndex_)
    return;

  if (!ftSize_)
    return;

  FT_Palette_Select(ftSize_->face,
                    static_cast<FT_UShort>(paletteIndex_),
                    &palette_);
  // XXX error handling
}


void
Engine::removeFont(int fontIndex,
                   bool closeFile)
{
  // We iterate over all triplets that contain the given font index
  // and remove them.
  QMap<FaceID, FTC_IDType>::iterator iter
    = faceIDMap_.lowerBound(FaceID(fontIndex, 0, 0));

  while (true)
  {
    if (iter == faceIDMap_.end())
      break;

    FaceID faceID = iter.key();
    if (faceID.fontIndex != fontIndex)
      break;

    auto ftcFaceID = reinterpret_cast<FTC_FaceID>(iter.value());
    FTC_Manager_RemoveFaceID(cacheManager_, ftcFaceID);

    iter = faceIDMap_.erase(iter);
  }

  if (closeFile)
    fontFileManager_.remove(fontIndex);
}


bool
Engine::currentFontBitmapOnly()
{
  if (!ftFallbackFace_)
    return false;
  return !FT_IS_SCALABLE(ftFallbackFace_);
}


bool
Engine::currentFontHasEmbeddedBitmap()
{
  if (!ftFallbackFace_)
    return false;
  return FT_HAS_FIXED_SIZES(ftFallbackFace_);
}


bool
Engine::currentFontHasColorLayers()
{
  if (!ftFallbackFace_)
    return false;
  return FT_HAS_COLOR(ftFallbackFace_);
}


bool
Engine::currentFontHasGlyphName()
{
  if (!ftFallbackFace_)
    return false;
  return FT_HAS_GLYPH_NAMES(ftFallbackFace_);
}


std::vector<int>
Engine::currentFontFixedSizes()
{
  if (!ftFallbackFace_
      || !FT_HAS_FIXED_SIZES(ftFallbackFace_)
      || !ftFallbackFace_->available_sizes)
    return {};

  std::vector<int> result;
  result.resize(ftFallbackFace_->num_fixed_sizes);
  // `x_ppem` is given in 26.6 fractional pixels.
  for (int i = 0; i < ftFallbackFace_->num_fixed_sizes; i++)
    result[i] = ftFallbackFace_->available_sizes[i].x_ppem >> 6;
  return result;
}


bool
Engine::currentFontPSInfo(PS_FontInfoRec& outInfo)
{
  if (!ftSize_)
    return false;
  if (FT_Get_PS_Font_Info(ftSize_->face, &outInfo) == FT_Err_Ok)
    return true;
  return false;
}


bool
Engine::currentFontPSPrivateInfo(PS_PrivateRec& outInfo)
{
  if (!ftSize_)
    return false;
  if (FT_Get_PS_Font_Private(ftSize_->face, &outInfo) == FT_Err_Ok)
    return true;
  return false;
}


std::vector<SFNTTableInfo>&
Engine::currentFontSFNTTableInfo()
{
  if (!curSFNTTablesValid_)
  {
    SFNTTableInfo::getForAll(this, curSFNTTables_);
    curSFNTTablesValid_ = true;
  }

  return curSFNTTables_;
}


int
Engine::currentFontFirstUnicodeCharMap()
{
  auto& charmaps = currentFontCharMaps();
  for (auto& cmap : charmaps)
    if (cmap.encoding == FT_ENCODING_UNICODE)
      return cmap.index;
  return -1;
}


unsigned
Engine::glyphIndexFromCharCode(int code,
                               int charMapIndex)
{
  if (charMapIndex < 0)
    return code;
  return FTC_CMapCache_Lookup(cmapCache_,
                              scaler_.face_id,
                              charMapIndex,
                              code);
}


FT_Pos
Engine::currentFontTrackingKerning(int degree)
{
  if (!ftSize_)
    return 0;

  FT_Pos result;
  // This function needs and returns points, not pixels.
  if (!FT_Get_Track_Kerning(ftSize_->face,
                            static_cast<FT_Fixed>(scaler_.width) << 10,
                            -degree,
                            &result))
  {
    result = static_cast<FT_Pos>((result / 1024.0 * scaler_.x_res) / 72.0);
    return result;
  }
  return 0;
}


FT_Vector
Engine::currentFontKerning(int glyphIndex,
                           int prevIndex)
{
  FT_Vector kern = {0, 0};
  FT_Get_Kerning(ftSize_->face,
                 prevIndex, glyphIndex,
                 FT_KERNING_UNFITTED, &kern);
  return kern;
}


std::pair<int, int>
Engine::currentSizeAscDescPx()
{
  return { ftSize_->metrics.ascender >> 6,
           ftSize_->metrics.descender >> 6 };
}


QString
Engine::glyphName(int index)
{
  QString name;

  if (index < 0)
    throw std::runtime_error("Invalid glyph index");

  reloadFont();
  if (ftFallbackFace_ && FT_HAS_GLYPH_NAMES(ftFallbackFace_))
  {
    char buffer[256];
    if (!FT_Get_Glyph_Name(ftFallbackFace_,
                           static_cast<unsigned int>(index),
                           buffer,
                           sizeof(buffer)))
      name = QString(buffer);
  }

  return name;
}


QString
Engine::dynamicLibraryVersion()
{
  int major, minor, patch;
  FT_Library_Version(library_, &major, &minor, &patch);
  return QString("%1.%2.%3")
           .arg(QString::number(major),
                QString::number(minor),
                QString::number(patch));
}


int
Engine::numberOfOpenedFonts()
{
  return fontFileManager_.size();
}


FT_Glyph
Engine::loadGlyph(int glyphIndex)
{
  update();

  if (glyphIndex < 0)
    throw std::runtime_error("Invalid glyph index");

  if (curNumGlyphs_ <= 0)
    return NULL;

  FT_Glyph glyph;

  // The `scaler` object is set up by the
  // `update` and `loadFont` methods.
  if (FTC_ImageCache_LookupScaler(imageCache_,
                                  &scaler_,
                                  loadFlags_,
                                  static_cast<unsigned int>(glyphIndex),
                                  &glyph,
                                  NULL))
  {
    // XXX error handling?
    return NULL;
  }

  return glyph;
}


int
Engine::loadGlyphIntoSlotWithoutCache(int glyphIndex,
                                      bool noScale)
{
  auto flags = static_cast<int>(loadFlags_);
  if (noScale)
    flags |= FT_LOAD_NO_SCALE;
  return FT_Load_Glyph(ftSize_->face, glyphIndex, flags);
}


// When continuous rendering, we don't need to call `update`.
// This is currently unused since the cache API doesn't support obtaining
// glyph metrics.  See `StringRenderer::loadSingleContext`.
FT_Glyph
Engine::loadGlyphWithoutUpdate(int glyphIndex,
                               FTC_Node* outNode,
                               bool forceRender)
{
  FT_Glyph glyph;
  auto oldFlags = imageType_.flags;
  if (forceRender)
    imageType_.flags |= FT_LOAD_RENDER;
  if (FTC_ImageCache_Lookup(imageCache_,
                            &imageType_,
                            glyphIndex,
                            &glyph,
                            outNode))
  {
    // XXX error handling?
    return NULL;
  }

  imageType_.flags = oldFlags;
  return glyph;
}


FT_Size_Metrics const&
Engine::currentFontMetrics()
{
  return ftSize_->metrics;
}


FT_GlyphSlot
Engine::currentFaceSlot()
{
  return ftSize_->face->glyph;
}


bool
Engine::renderReady()
{
  return ftSize_ != NULL;
}


bool
Engine::fontValid()
{
  return ftFallbackFace_ != NULL;
}


void
Engine::openFonts(QStringList const& fontFileNames)
{
  fontFileManager_.append(fontFileNames, true);
}


void
Engine::setSizeByPixel(double pixelSize)
{
  this->pixelSize_ = pixelSize;
  pointSize_ = pixelSize * 72.0 / dpi_;
  usingPixelSize_ = true;
}


void
Engine::setSizeByPoint(double pointSize)
{
  this->pointSize_ = pointSize;
  pixelSize_ = pointSize * dpi_ / 72.0;
  usingPixelSize_ = false;
}


void
Engine::setLcdFilter(FT_LcdFilter filter)
{
  FT_Library_SetLcdFilter(library_, filter);
}


void
Engine::setCFFHintingMode(int mode)
{
  FT_Error error = FT_Property_Set(library_,
                                   "cff",
                                   "hinting-engine",
                                   &mode);
  if (!error)
    resetCache();
}


void
Engine::setTTInterpreterVersion(int version)
{
  FT_Error error = FT_Property_Set(library_,
                                   "truetype",
                                   "interpreter-version",
                                   &version);
  if (!error)
    resetCache();
}


void
Engine::setStemDarkening(bool darkening)
{
  FT_Bool noDarkening = !darkening;
  FT_Property_Set(library_,
                  "cff",
                  "no-stem-darkening",
                  &noDarkening);
  FT_Property_Set(library_,
                  "autofitter",
                  "no-stem-darkening",
                  &noDarkening);
  FT_Property_Set(library_,
                  "type1",
                  "no-stem-darkening",
                  &noDarkening);
  FT_Property_Set(library_,
                  "t1cid",
                  "no-stem-darkening",
                  &noDarkening);
  resetCache();
}


void
Engine::applyMMGXDesignCoords(FT_Fixed* coords,
                              size_t count)
{
  if (!ftSize_)
    return;
  if (count >= UINT_MAX)
    count = UINT_MAX - 1;
  FT_Set_Var_Design_Coordinates(ftSize_->face,
                                static_cast<unsigned>(count),
                                coords);
}


void
Engine::update()
{
  loadFlags_ = FT_LOAD_DEFAULT;

  if (!embeddedBitmap_)
    loadFlags_ |= FT_LOAD_NO_BITMAP;

  if (doHinting_)
  {
    loadFlags_ |= antiAliasingTarget_;
    if (doAutoHinting_)
      loadFlags_ |= FT_LOAD_FORCE_AUTOHINT;
  }
  else
  {
    loadFlags_ |= FT_LOAD_NO_HINTING;
    // When users disable hinting for tricky fonts,
    // we assume that they *really* want to disable it.
    if (currentFontTricky())
      loadFlags_ |= FT_LOAD_NO_AUTOHINT;

    if (!antiAliasingEnabled_)
      loadFlags_ |= FT_LOAD_MONOCHROME;
  }

  if (useColorLayer_ && embeddedBitmap_ && currentFontHasEmbeddedBitmap())
    loadFlags_ |= FT_LOAD_COLOR; // XXX probably bug: undesired color rendering

  scaler_.pixel = 0; // Use 26.6 format.

  if (usingPixelSize_)
  {
    scaler_.width = static_cast<unsigned int>(pixelSize_ * 64.0);
    scaler_.height = static_cast<unsigned int>(pixelSize_ * 64.0);
    scaler_.x_res = 0;
    scaler_.y_res = 0;
  }
  else
  {
    scaler_.width = static_cast<unsigned int>(pointSize_ * 64.0);
    scaler_.height = static_cast<unsigned int>(pointSize_ * 64.0);
    scaler_.x_res = dpi_;
    scaler_.y_res = dpi_;
  }

  imageType_.width = static_cast<unsigned int>(pixelSize_);
  imageType_.height = static_cast<unsigned int>(pixelSize_);
  imageType_.flags = static_cast<int>(loadFlags_);
}


void
Engine::resetCache()
{
  // Reset the cache.
  FTC_Manager_Reset(cacheManager_);
  ftFallbackFace_ = NULL;
  ftSize_ = NULL;
  palette_ = NULL;
}


void
Engine::loadDefaults()
{
  if (fontType_ == FontType_CFF)
    setCFFHintingMode(engineDefaults_.cffHintingEngineDefault);
  else if (fontType_ == FontType_TrueType)
  {
    if (currentFontTricky())
      setTTInterpreterVersion(TT_INTERPRETER_VERSION_35);
    else
      setTTInterpreterVersion(engineDefaults_.ttInterpreterVersionDefault);
  }
  setStemDarkening(false);
  applyMMGXDesignCoords(NULL, 0);

  setAntiAliasingEnabled(true);
  setAntiAliasingTarget(FT_LOAD_TARGET_NORMAL);
  setHinting(true);
  setAutoHinting(false);
  setEmbeddedBitmapEnabled(true);
  setPaletteIndex(0);
  setUseColorLayer(true);

  renderingEngine()->setBackground(qRgba(255, 255, 255, 255));
  renderingEngine()->setForeground(qRgba(0, 0, 0, 255));
  renderingEngine()->setGamma(1.8);

  resetCache();
  reloadFont();
  loadPalette();
}


void
Engine::queryEngine()
{
  FT_Error error;

  // Query engines and check for alternatives.

  // CFF
  error = FT_Property_Get(library_,
                          "cff",
                          "hinting-engine",
                          &engineDefaults_.cffHintingEngineDefault);
  if (error)
  {
    // No CFF engine.
    engineDefaults_.cffHintingEngineDefault = -1;
    engineDefaults_.cffHintingEngineOther = -1;
  }
  else
  {
    int engines[] =
    {
      FT_HINTING_FREETYPE,
      FT_HINTING_ADOBE
    };

    int i;
    for (i = 0; i < 2; i++)
      if (engineDefaults_.cffHintingEngineDefault == engines[i])
        break;

    engineDefaults_.cffHintingEngineOther = engines[(i + 1) % 2];

    error = FT_Property_Set(library_,
                            "cff",
                            "hinting-engine",
                            &engineDefaults_.cffHintingEngineOther);
    if (error)
      engineDefaults_.cffHintingEngineOther = -1;

    // Reset.
    FT_Property_Set(library_,
                    "cff",
                    "hinting-engine",
                    &engineDefaults_.cffHintingEngineDefault);
  }

  // TrueType
  error = FT_Property_Get(library_,
                          "truetype",
                          "interpreter-version",
                          &engineDefaults_.ttInterpreterVersionDefault);
  if (error)
  {
    // No TrueType engine.
    engineDefaults_.ttInterpreterVersionDefault = -1;
    engineDefaults_.ttInterpreterVersionOther = -1;
    engineDefaults_.ttInterpreterVersionOther1 = -1;
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
      if (engineDefaults_.ttInterpreterVersionDefault == interpreters[i])
        break;

    engineDefaults_.ttInterpreterVersionOther = interpreters[(i + 1) % 3];

    error = FT_Property_Set(library_,
                            "truetype",
                            "interpreter-version",
                            &engineDefaults_.ttInterpreterVersionOther);
    if (error)
      engineDefaults_.ttInterpreterVersionOther = -1;

    engineDefaults_.ttInterpreterVersionOther1 = interpreters[(i + 2) % 3];

    error = FT_Property_Set(library_,
                            "truetype",
                            "interpreter-version",
                            &engineDefaults_.ttInterpreterVersionOther1);
    if (error)
      engineDefaults_.ttInterpreterVersionOther1 = -1;

    // Reset.
    FT_Property_Set(library_,
                    "truetype",
                    "interpreter-version",
                    &engineDefaults_.ttInterpreterVersionDefault);
  }
}


void
Engine::loadPaletteInfos()
{
  curPaletteInfos_.clear();

  if (FT_Palette_Data_Get(ftFallbackFace_, &paletteData_))
  {
    // XXX Error handling
    paletteData_.num_palettes = 0;
    return;
  }

  // The size never exceeds the maximum value of `unsigned short`.
  curPaletteInfos_.reserve(paletteData_.num_palettes);
  for (int i = 0; i < paletteData_.num_palettes; ++i)
    curPaletteInfos_.emplace_back(ftFallbackFace_,
                                  paletteData_,
                                  i,
                                  &curSFNTNames_);
}


// end of engine.cpp
