// engine.cpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#include "engine.hpp"

#include <stdexcept>
#include <stdint.h>

#include <freetype/ftmodapi.h>
#include <freetype/ftdriver.h>
#include <freetype/ftlcdfil.h>


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
  Engine* engine = static_cast<Engine*>(requestData);
  // `ftcFaceID' is actually an integer
  // -> first convert pointer to same-width integer, then discard superfluous
  //    bits (e.g., on x86_64 where pointers are wider than int)
  int val = static_cast<int>(reinterpret_cast<intptr_t>(ftcFaceID));
  // make sure this does not cause information loss
  Q_ASSERT_X(sizeof(void*) >= sizeof(int),
             "faceRequester",
             "Pointer size must be at least the size of int"
             " in order to treat FTC_FaceID correctly");

  const FaceID& faceID = engine->faceIDMap_.key(val);

  // this is the only place where we have to check the validity of the font
  // index; note that the validity of both the face and named instance index
  // is checked by FreeType itself
  if (faceID.fontIndex < 0
      || faceID.fontIndex >= engine->numberOfOpenedFonts())
    return FT_Err_Invalid_Argument;

  QString font = engine->fontFileManager_[faceID.fontIndex].filePath();
  long faceIndex = faceID.faceIndex;

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

Engine::Engine()
{
  ftSize_ = NULL;
  // we reserve value 0 for the `invalid face ID'
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
}


Engine::~Engine()
{
  FTC_Manager_Done(cacheManager_);
  FT_Done_FreeType(library_);
}


long
Engine::numberOfFaces(int fontIndex)
{
  FT_Face face;
  long numFaces = -1;

  // search triplet (fontIndex, 0, 0)
  FTC_FaceID ftcFaceID = reinterpret_cast<FTC_FaceID>
                           (faceIDMap_.value(FaceID(fontIndex,
                                                   0,
                                                   0)));
  if (ftcFaceID)
  {
    // found
    if (!FTC_Manager_LookupFace(cacheManager_, ftcFaceID, &face))
      numFaces = face->num_faces;
  }
  else
  {
    // not found; try to load triplet (fontIndex, 0, 0)
    ftcFaceID = reinterpret_cast<FTC_FaceID>(faceCounter_);
    faceIDMap_.insert(FaceID(fontIndex, 0, 0),
                     faceCounter_++);

    if (!FTC_Manager_LookupFace(cacheManager_, ftcFaceID, &face))
      numFaces = face->num_faces;
    else
    {
      faceIDMap_.remove(FaceID(fontIndex, 0, 0));
      faceCounter_--;
    }
  }

  return numFaces;
}


int
Engine::numberOfNamedInstances(int fontIndex,
                               long faceIndex)
{
  FT_Face face;
  // we return `n' named instances plus one;
  // instance index 0 represents a face without a named instance selected
  int numNamedInstances = -1;

  // search triplet (fontIndex, faceIndex, 0)
  FTC_FaceID ftcFaceID = reinterpret_cast<FTC_FaceID>
                           (faceIDMap_.value(FaceID(fontIndex,
                                                   faceIndex,
                                                   0)));
  if (ftcFaceID)
  {
    // found
    if (!FTC_Manager_LookupFace(cacheManager_, ftcFaceID, &face))
      numNamedInstances = static_cast<int>((face->style_flags >> 16) + 1);
  }
  else
  {
    // not found; try to load triplet (fontIndex, faceIndex, 0)
    ftcFaceID = reinterpret_cast<FTC_FaceID>(faceCounter_);
    faceIDMap_.insert(FaceID(fontIndex, faceIndex, 0),
                     faceCounter_++);

    if (!FTC_Manager_LookupFace(cacheManager_, ftcFaceID, &face))
      numNamedInstances = static_cast<int>((face->style_flags >> 16) + 1);
    else
    {
      faceIDMap_.remove(FaceID(fontIndex, faceIndex, 0));
      faceCounter_--;
    }
  }

  return numNamedInstances;
}


int
Engine::loadFont(int fontIndex,
                 long faceIndex,
                 int namedInstanceIndex)
{
  int numGlyphs = -1;
  fontType_ = FontType_Other;

  update();

  // search triplet (fontIndex, faceIndex, namedInstanceIndex)
  scaler_.face_id = reinterpret_cast<FTC_FaceID>
                     (faceIDMap_.value(FaceID(fontIndex,
                                             faceIndex,
                                             namedInstanceIndex)));
  if (scaler_.face_id)
  {
    // found
    if (!FTC_Manager_LookupSize(cacheManager_, &scaler_, &ftSize_))
      numGlyphs = ftSize_->face->num_glyphs;
  }
  else
  {
    // not found; try to load triplet
    // (fontIndex, faceIndex, namedInstanceIndex)
    scaler_.face_id = reinterpret_cast<FTC_FaceID>(faceCounter_);
    faceIDMap_.insert(FaceID(fontIndex,
                            faceIndex,
                            namedInstanceIndex),
                     faceCounter_++);

    if (!FTC_Manager_LookupSize(cacheManager_, &scaler_, &ftSize_))
      numGlyphs = ftSize_->face->num_glyphs;
    else
    {
      faceIDMap_.remove(FaceID(fontIndex,
                              faceIndex,
                              namedInstanceIndex));
      faceCounter_--;
    }
  }

  imageType_.face_id = scaler_.face_id;

  if (numGlyphs < 0)
  {
    ftSize_ = NULL;
    curFamilyName_ = QString();
    curStyleName_ = QString();
  }
  else
  {
    auto face = ftSize_->face;
    curFamilyName_ = QString(face->family_name);
    curStyleName_ = QString(face->style_name);

    const char* moduleName = FT_FACE_DRIVER_NAME( face );

    // XXX cover all available modules
    if (!strcmp(moduleName, "cff"))
      fontType_ = FontType_CFF;
    else if (!strcmp(moduleName, "truetype"))
      fontType_ = FontType_TrueType;

    curCharMaps_.clear();
    curCharMaps_.reserve(face->num_charmaps);
    for (int i = 0; i < face->num_charmaps; i++)
      curCharMaps_.append(CharMapInfo(i, face->charmaps[i]));
  }

  curNumGlyphs_ = numGlyphs;
  return numGlyphs;
}


void
Engine::reloadFont()
{
  update();
  if (!scaler_.face_id)
    return;
  imageType_.face_id = scaler_.face_id;
  FTC_Manager_LookupSize(cacheManager_, &scaler_, &ftSize_);
}


void
Engine::removeFont(int fontIndex, bool closeFile)
{
  // we iterate over all triplets that contain the given font index
  // and remove them
  QMap<FaceID, FTC_IDType>::iterator iter
    = faceIDMap_.lowerBound(FaceID(fontIndex, 0, 0));

  for (;;)
  {
    if (iter == faceIDMap_.end())
      break;

    FaceID faceID = iter.key();
    if (faceID.fontIndex != fontIndex)
      break;

    FTC_FaceID ftcFaceID = reinterpret_cast<FTC_FaceID>(iter.value());
    FTC_Manager_RemoveFaceID(cacheManager_, ftcFaceID);

    iter = faceIDMap_.erase(iter);
  }

  if (closeFile)
    fontFileManager_.remove(fontIndex);
}


unsigned
Engine::glyphIndexFromCharCode(int code, int charMapIndex)
{
  if (charMapIndex == -1)
    return code;
  return FTC_CMapCache_Lookup(cmapCache_, scaler_.face_id, charMapIndex, code);
}


FT_Size_Metrics const&
Engine::currentFontMetrics()
{
  return ftSize_->metrics;
}


QString
Engine::glyphName(int index)
{
  QString name;

  if (index < 0)
    throw std::runtime_error("Invalid glyph index");

   if (!FTC_Manager_LookupSize(cacheManager_, &scaler_, &ftSize_))
    return name;

  if (ftSize_ && FT_HAS_GLYPH_NAMES(ftSize_->face))
  {
    char buffer[256];
    if (!FT_Get_Glyph_Name(ftSize_->face,
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
  if (FTC_ImageCache_LookupScaler(imageCache_,
                                  &scaler_,
                                  loadFlags_ | FT_LOAD_NO_BITMAP,
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


FT_Glyph
Engine::loadGlyphWithoutUpdate(int glyphIndex)
{
  // TODO bitmap fonts? color layered fonts?
  FT_Glyph glyph;
  imageType_.flags |= FT_LOAD_NO_BITMAP;
  if (FTC_ImageCache_Lookup(imageCache_,
                            &imageType_,
                            glyphIndex,
                            &glyph,
                            NULL))
  {
    // XXX error handling?
    return NULL;
  }

  return glyph;
}


int
Engine::numberOfOpenedFonts()
{
  return fontFileManager_.size();
}


void
Engine::openFonts(QStringList fontFileNames)
{
  fontFileManager_.append(fontFileNames);
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
  {
    // reset the cache
    FTC_Manager_Reset(cacheManager_);
    ftSize_ = NULL;
  }
}


void
Engine::setTTInterpreterVersion(int version)
{
  FT_Error error = FT_Property_Set(library_,
                                   "truetype",
                                   "interpreter-version",
                                   &version);
  if (!error)
  {
    // reset the cache
    FTC_Manager_Reset(cacheManager_);
    ftSize_ = NULL;
  }
}


void
Engine::update()
{
  loadFlags_ = FT_LOAD_DEFAULT;
  if (doAutoHinting_)
    loadFlags_ |= FT_LOAD_FORCE_AUTOHINT;
  loadFlags_ |= FT_LOAD_NO_BITMAP; // XXX handle bitmap fonts also

  if (doHinting_)
  {
    // TODO Differentiate RGB/BGR here?
    unsigned long target = antiAliasingTarget_;
    loadFlags_ |= target;
  }
  else
  {
    loadFlags_ |= FT_LOAD_NO_HINTING;

    if (!antiAliasingEnabled_) // XXX does this hold?
      loadFlags_ |= FT_LOAD_MONOCHROME;
  }

  // XXX handle color fonts also

  scaler_.pixel = 0; // use 26.6 format

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
Engine::queryEngine()
{
  FT_Error error;

  // query engines and check for alternatives

  // CFF
  error = FT_Property_Get(library_,
                          "cff",
                          "hinting-engine",
                          &engineDefaults_.cffHintingEngineDefault);
  if (error)
  {
    // no CFF engine
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

    // reset
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
    // no TrueType engine
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

    // reset
    FT_Property_Set(library_,
                    "truetype",
                    "interpreter-version",
                    &engineDefaults_.ttInterpreterVersionDefault);
  }
}


QHash<FT_Encoding, QString> encodingNamesCache;
QHash<FT_Encoding, QString>&
encodingNames()
{
  if (encodingNamesCache.empty())
  {
    encodingNamesCache[static_cast<FT_Encoding>(FT_ENCODING_OTHER)]
     = "Unknown Encoding";
    encodingNamesCache[FT_ENCODING_NONE] = "No Encoding";
    encodingNamesCache[FT_ENCODING_MS_SYMBOL] = "MS Symbol (symb)";
    encodingNamesCache[FT_ENCODING_UNICODE] = "Unicode (unic)";
    encodingNamesCache[FT_ENCODING_SJIS] = "Shift JIS (sjis)";
    encodingNamesCache[FT_ENCODING_PRC] = "PRC/GB 18030 (gb)";
    encodingNamesCache[FT_ENCODING_BIG5] = "Big5 (big5)";
    encodingNamesCache[FT_ENCODING_WANSUNG] = "Wansung (wans)";
    encodingNamesCache[FT_ENCODING_JOHAB] = "Johab (joha)";
    encodingNamesCache[FT_ENCODING_ADOBE_STANDARD] = "Adobe Standard (ADOB)";
    encodingNamesCache[FT_ENCODING_ADOBE_EXPERT] = "Adobe Expert (ADBE)";
    encodingNamesCache[FT_ENCODING_ADOBE_CUSTOM] = "Adobe Custom (ADBC)";
    encodingNamesCache[FT_ENCODING_ADOBE_LATIN_1] = "Latin 1 (lat1)";
    encodingNamesCache[FT_ENCODING_OLD_LATIN_2] = "Latin 2 (lat2)";
    encodingNamesCache[FT_ENCODING_APPLE_ROMAN] = "Apple Roman (armn)";
  }

  return encodingNamesCache;
}


CharMapInfo::CharMapInfo(int index, FT_CharMap cmap)
: index(index), ptr(cmap), encoding(cmap->encoding), maxIndex(-1)
{
  auto& names = encodingNames();
  auto it = names.find(encoding);
  if (it == names.end())
    encodingName = &names[static_cast<FT_Encoding>(FT_ENCODING_OTHER)];
  else
    encodingName = &it.value();

  if (encoding != FT_ENCODING_OTHER)
    maxIndex = computeMaxIndex();
}


QString
CharMapInfo::stringifyIndex(int code, int index)
{
  return QString("CharCode: %1 (glyph idx %2)")
           .arg(stringifyIndexShort(code))
           .arg(index);
}


QString
CharMapInfo::stringifyIndexShort(int code)
{
  return (encoding == FT_ENCODING_UNICODE ? "U+" : "0x")
         + QString::number(code, 16).rightJustified(4, '0').toUpper();
}


int
CharMapInfo::computeMaxIndex()
{
  int maxIndex = 0;
  switch (encoding)
  {
  case FT_ENCODING_UNICODE:
    maxIndex = maxIndexForFaceAndCharMap(ptr, 0x110000) + 1;
    break;

  case FT_ENCODING_ADOBE_LATIN_1:
  case FT_ENCODING_ADOBE_STANDARD:
  case FT_ENCODING_ADOBE_EXPERT:
  case FT_ENCODING_ADOBE_CUSTOM:
  case FT_ENCODING_APPLE_ROMAN:
    maxIndex = 0x100;
    break;

  /* some fonts use range 0x00-0x100, others have 0xF000-0xF0FF */
  case FT_ENCODING_MS_SYMBOL:
    maxIndex = maxIndexForFaceAndCharMap(ptr, 0x10000) + 1;
    break;

  default:
    // Some encodings can reach > 0x10000, e.g. GB 18030.
    maxIndex = maxIndexForFaceAndCharMap(ptr, 0x110000) + 1;
  }
  return maxIndex;
}


int
CharMapInfo::maxIndexForFaceAndCharMap(FT_CharMap charMap,
                                       unsigned max)
{
  // code adopted from `ftcommon.c`
  FT_ULong min = 0;
  FT_UInt glyphIndex;
  FT_Face face = charMap->face;

  if (FT_Set_Charmap(face, charMap))
    return -1;

  do
  {
    FT_ULong mid = (min + max) >> 1;
    FT_ULong res = FT_Get_Next_Char(face, mid, &glyphIndex);

    if (glyphIndex)
      min = res;
    else
    {
      max = mid;

      // once moved, it helps to advance min through sparse regions
      if (min)
      {
        res = FT_Get_Next_Char(face, min, &glyphIndex);

        if (glyphIndex)
          min = res;
        else
          max = min; // found it
      }
    }
  } while (max > min);

  return static_cast<int>(max);
}


// end of engine.cpp
