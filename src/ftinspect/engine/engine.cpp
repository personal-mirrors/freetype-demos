// engine.cpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#include "engine.hpp"

#include "../rendering/renderutils.hpp"
#include "../rendering/graphicsdefault.hpp"

#include <stdexcept>
#include <stdint.h>

#include <freetype/ftmodapi.h>
#include <freetype/ftdriver.h>
#include <freetype/ftlcdfil.h>
#include <freetype/ftbitmap.h>


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

  if (fontIndex < 0)
    return -1;

  auto id = FaceID(fontIndex, 0, 0);

  // search triplet (fontIndex, 0, 0)
  FTC_FaceID ftcFaceID = reinterpret_cast<FTC_FaceID>(faceIDMap_.value(id));
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
    faceIDMap_.insert(id, faceCounter_++);

    if (!FTC_Manager_LookupFace(cacheManager_, ftcFaceID, &face))
      numFaces = face->num_faces;
    else
    {
      faceIDMap_.remove(id);
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
  if (fontIndex < 0)
    return -1;
  auto id = FaceID(fontIndex, faceIndex, 0);

  // search triplet (fontIndex, faceIndex, 0)
  FTC_FaceID ftcFaceID = reinterpret_cast<FTC_FaceID>(faceIDMap_.value(id));
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
    faceIDMap_.insert(id, faceCounter_++);

    if (!FTC_Manager_LookupFace(cacheManager_, ftcFaceID, &face))
      numNamedInstances = static_cast<int>((face->style_flags >> 16) + 1);
    else
    {
      faceIDMap_.remove(id);
      faceCounter_--;
    }
  }

  return numNamedInstances;
}


QString
Engine::namedInstanceName(int fontIndex, long faceIndex, int index)
{
  FT_Face face;
  QString name;

  if (fontIndex < 0)
    return QString();

  auto id = FaceID(fontIndex, faceIndex, index);

  // search triplet (fontIndex, faceIndex, 0)
  FTC_FaceID ftcFaceID = reinterpret_cast<FTC_FaceID>(faceIDMap_.value(id));
  if (ftcFaceID)
  {
    // found
    if (!FTC_Manager_LookupFace(cacheManager_, ftcFaceID, &face))
      name = QString("%1 %2")
               .arg(face->family_name)
               .arg(face->style_name);
  }
  else
  {
    // not found; try to load triplet (fontIndex, faceIndex, 0)
    ftcFaceID = reinterpret_cast<FTC_FaceID>(faceCounter_);
    faceIDMap_.insert(id, faceCounter_++);

    if (!FTC_Manager_LookupFace(cacheManager_, ftcFaceID, &face))
      name = QString("%1 %2")
               .arg(face->family_name)
               .arg(face->style_name);
    else
    {
      faceIDMap_.remove(id);
      faceCounter_--;
    }
  }

  return name;
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


int
Engine::loadFont(int fontIndex,
                 long faceIndex,
                 int namedInstanceIndex)
{
  int numGlyphs = -1;
  fontType_ = FontType_Other;

  update();

  auto id = FaceID(fontIndex, faceIndex, namedInstanceIndex);

  // search triplet (fontIndex, faceIndex, namedInstanceIndex)
  scaler_.face_id = reinterpret_cast<FTC_FaceID>(faceIDMap_.value(id));
  if (scaler_.face_id)
  {
    // found
    if (!FTC_Manager_LookupSize(cacheManager_, &scaler_, &ftSize_))
      numGlyphs = ftSize_->face->num_glyphs;
  }
  else if (fontIndex >= 0)
  {
    // not found; try to load triplet
    // (fontIndex, faceIndex, namedInstanceIndex)
    scaler_.face_id = reinterpret_cast<FTC_FaceID>(faceCounter_);
    faceIDMap_.insert(id, faceCounter_++);

    if (!FTC_Manager_LookupSize(cacheManager_, &scaler_, &ftSize_))
      numGlyphs = ftSize_->face->num_glyphs;
    else
    {
      faceIDMap_.remove(id);
      faceCounter_--;
    }
  }

  imageType_.face_id = scaler_.face_id;

  if (numGlyphs < 0)
  {
    ftSize_ = NULL;
    curFamilyName_ = QString();
    curStyleName_ = QString();

    curCharMaps_.clear();
    curPaletteInfos_.clear();
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
      curCharMaps_.emplace_back(i, face->charmaps[i]);

    loadPaletteInfos();
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
Engine::loadPalette()
{
  palette_ = NULL;
  if (paletteData_.num_palettes == 0
      || paletteIndex_ < 0
      || paletteData_.num_palettes <= paletteIndex_)
    return;

  FT_Palette_Select(ftSize_->face, 
                    static_cast<FT_UShort>(paletteIndex_),
                    &palette_);
  // XXX error handling
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
  if (charMapIndex < 0)
    return code;
  return FTC_CMapCache_Lookup(cmapCache_, scaler_.face_id, charMapIndex, code);
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


FT_Pos
Engine::currentFontTrackingKerning(int degree)
{
  FT_Pos result;
  // this function needs and returns points, not pixels
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


QString
Engine::glyphName(int index)
{
  QString name;

  if (index < 0)
    throw std::runtime_error("Invalid glyph index");

  if (!ftSize_)
    return "";

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


FT_Glyph
Engine::loadGlyph(int glyphIndex)
{
  update();

  if (glyphIndex < 0)
    throw std::runtime_error("Invalid glyph index");

  if (curNumGlyphs_ <= 0)
    return NULL;

  FT_Glyph glyph;

  // the `scaler' object is set up by the
  // `update' and `loadFont' methods
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
Engine::loadGlyphIntoSlotWithoutCache(int glyphIndex)
{
  return FT_Load_Glyph(ftSize_->face, glyphIndex, loadFlags_);
}


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


bool
Engine::convertGlyphToBitmapGlyph(FT_Glyph src,
                      FT_Glyph* out)
{
  if (src->format == FT_GLYPH_FORMAT_BITMAP)
  {
    *out = src;
    return false;
  }
  if (src->format != FT_GLYPH_FORMAT_OUTLINE)
  {
    *out = NULL;
    return false;
    // TODO support SVG
  }

  if (src->format == FT_GLYPH_FORMAT_OUTLINE)
  {
    FT_Glyph out2 = src;
    auto error = FT_Glyph_To_Bitmap(&out2, 
                                    static_cast<FT_Render_Mode>(renderMode_),
                                    nullptr,
                                    false);
    if (error)
    {
      *out = NULL;
      return false;
    }
    *out = out2;
    return true;
  }

  *out = NULL;
  return false;
}


FT_Bitmap
Engine::convertBitmapTo8Bpp(FT_Bitmap* bitmap)
{
  FT_Bitmap out;
  out.buffer = NULL;
  auto error = FT_Bitmap_Convert(library_, bitmap, &out, 1);
  if (error)
  {
    // XXX handling?
  }
  return out;
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

  if (!embeddedBitmap_)
    loadFlags_ |= FT_LOAD_NO_BITMAP;

  if (doHinting_)
  {
    loadFlags_ |= antiAliasingTarget_;
  }
  else
  {
    loadFlags_ |= FT_LOAD_NO_HINTING;

    if (!antiAliasingEnabled_)
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


void
Engine::loadPaletteInfos()
{
  curPaletteInfos_.clear();

  if (FT_Palette_Data_Get(ftSize_->face, &paletteData_))
  {
    // XXX Error handling
    paletteData_.num_palettes = 0;
    return;
  }

  curPaletteInfos_.reserve(paletteData_.num_palettes);
  for (int i = 0; i < paletteData_.num_palettes; ++i)
    curPaletteInfos_.emplace_back(ftSize_->face, paletteData_, i);
}


void
convertLCDToARGB(FT_Bitmap& bitmap,
                 QImage& image,
                 bool isBGR)
{
  // TODO to be implemented
}


void
convertLCDVToARGB(FT_Bitmap& bitmap,
                  QImage& image,
                  bool isBGR)
{
  // TODO to be implemented
}


QImage*
Engine::convertBitmapToQImage(FT_Bitmap* src)
{
  QImage* result = NULL;
  
  auto& bmap = *src;
  bool ownBitmap = false;

  int width = bmap.width;
  int height = bmap.rows;
  QImage::Format format = QImage::Format_Indexed8; // goto crossing init

  if (bmap.pixel_mode == FT_PIXEL_MODE_GRAY2
      || bmap.pixel_mode == FT_PIXEL_MODE_GRAY4)
  {
    bmap = convertBitmapTo8Bpp(&bmap);
    if (!bmap.buffer)
      goto cleanup;
    ownBitmap = true;
  }

  if (bmap.pixel_mode == FT_PIXEL_MODE_LCD)
    width /= 3;
  else if (bmap.pixel_mode == FT_PIXEL_MODE_LCD_V)
    height /= 3;

  switch (bmap.pixel_mode)
  {
  case FT_PIXEL_MODE_MONO:
    format = QImage::Format_Mono;
    break;
  case FT_PIXEL_MODE_GRAY:
    format = QImage::Format_Indexed8;
    break;
  case FT_PIXEL_MODE_BGRA:
    // XXX "ARGB" here means BGRA due to endianness - may be problematic
    // on big-endian machines
    format = QImage::Format_ARGB32_Premultiplied;
    break;
  case FT_PIXEL_MODE_LCD:
  case FT_PIXEL_MODE_LCD_V:
    format = QImage::Format_ARGB32;
    break;
  default:
    goto cleanup;
  }

  switch (bmap.pixel_mode) 
  {
  case FT_PIXEL_MODE_MONO:
  case FT_PIXEL_MODE_GRAY:
  case FT_PIXEL_MODE_BGRA:
    {
      QImage image(bmap.buffer, 
                   width, height, 
                   bmap.pitch, 
                   format);
      if (bmap.pixel_mode == FT_PIXEL_MODE_GRAY)
        image.setColorTable(GraphicsDefault::deafultInstance()->grayColorTable);
      else if (bmap.pixel_mode == FT_PIXEL_MODE_MONO)
        image.setColorTable(GraphicsDefault::deafultInstance()->monoColorTable);
      result = new QImage(image.copy());
      // Don't directly use `image` since we're destroying the image
    }
    break;
  case FT_PIXEL_MODE_LCD:;
    result = new QImage(width, height, format);
    convertLCDToARGB(bmap, *result, lcdUsesBGR_);
    break;
  case FT_PIXEL_MODE_LCD_V:;
    result = new QImage(width, height, format);
    convertLCDVToARGB(bmap, *result, lcdUsesBGR_);
    break;
  }

cleanup:
  if (ownBitmap)
    FT_Bitmap_Done(library_, &bmap);

  return result;
}


QImage*
Engine::convertGlyphToQImage(FT_Glyph src,
                             QRect* outRect,
                             bool inverseRectY)
{
  FT_BitmapGlyph bitmapGlyph;
  bool ownBitmapGlyph
    = convertGlyphToBitmapGlyph(src, reinterpret_cast<FT_Glyph*>(&bitmapGlyph));
  if (!bitmapGlyph)
    return NULL;

  auto result = convertBitmapToQImage(&bitmapGlyph->bitmap);

  if (result && outRect)
  {
    outRect->setLeft(bitmapGlyph->left);
    if (inverseRectY)
      outRect->setTop(-bitmapGlyph->top);
    else
      outRect->setTop(bitmapGlyph->top);
    outRect->setWidth(bitmapGlyph->bitmap.width);
    outRect->setHeight(bitmapGlyph->bitmap.rows);
  }

  if (ownBitmapGlyph)
    FT_Done_Glyph(reinterpret_cast<FT_Glyph>(bitmapGlyph));

  return result;
}


QPoint
Engine::computeGlyphOffset(FT_Glyph glyph, bool inverseY)
{
  if (glyph->format == FT_GLYPH_FORMAT_OUTLINE)
  {
    FT_BBox cbox;
    FT_Outline_Get_CBox(&reinterpret_cast<FT_OutlineGlyph>(glyph)->outline, 
                        &cbox);
    cbox.xMin &= ~63;
    cbox.yMin &= ~63;
    cbox.xMax = (cbox.xMax + 63) & ~63;
    cbox.yMax = (cbox.yMax + 63) & ~63;
    if (inverseY)
      cbox.yMax = -cbox.yMax;
    return { static_cast<int>(cbox.xMin) / 64,
               static_cast<int>(cbox.yMax / 64) };
  }
  if (glyph->format == FT_GLYPH_FORMAT_BITMAP)
  {
    auto bg = reinterpret_cast<FT_BitmapGlyph>(glyph);
    if (inverseY)
      return { bg->left, -bg->top };
    return { bg->left, bg->top };
  }

  return {};
}


QImage*
Engine::tryDirectRenderColorLayers(int glyphIndex,
                                   QRect* outRect)
{
  if (palette_ == NULL 
      || !useColorLayer_ 
      || paletteIndex_ >= paletteData_.num_palettes)
    return NULL;

  FT_LayerIterator iter = {};
  
  FT_UInt layerGlyphIdx = 0;
  FT_UInt layerColorIdx = 0;

  bool next = FT_Get_Color_Glyph_Layer(ftSize_->face,
                                       glyphIndex,
                                       &layerGlyphIdx,
                                       &layerColorIdx,
                                       &iter);
  if (!next)
    return NULL;

  // temporarily change lf
  auto oldLoadFlags = imageType_.flags;
  auto loadFlags = oldLoadFlags;
  loadFlags &= ~FT_LOAD_COLOR;
  loadFlags |= FT_LOAD_RENDER;

  loadFlags &= ~FT_LOAD_TARGET_(0xF);
  loadFlags |= FT_LOAD_TARGET_NORMAL;
  imageType_.flags = loadFlags;

  FT_Bitmap bitmap = {};
  FT_Bitmap_Init(&bitmap);

  FT_Vector bitmapOffset = {};
  bool failed = false;

  do
  {
    FT_Vector slotOffset;
    FT_Glyph glyph;
    if (FTC_ImageCache_Lookup(imageCache_,
                              &imageType_,
                              layerGlyphIdx,
                              &glyph,
                              NULL))
    {
      // XXX Error handling
      failed = true;
      break;
    }

    if (glyph->format != FT_GLYPH_FORMAT_BITMAP)
      continue;

    auto bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
    slotOffset.x = bitmapGlyph->left << 6;
    slotOffset.y = bitmapGlyph->top << 6;

    FT_Color color = {};

    if (layerColorIdx == 0xFFFF)
    {
      // TODO: FT_Palette_Get_Foreground_Color: #1134
      if (paletteData_.palette_flags
          && (paletteData_.palette_flags[paletteIndex_] 
              & FT_PALETTE_FOR_DARK_BACKGROUND))
      {
        /* white opaque */
        color.blue = 0xFF;
        color.green = 0xFF;
        color.red = 0xFF;
        color.alpha = 0xFF;
      }
      else
      {
        /* black opaque */
        color.blue = 0x00;
        color.green = 0x00;
        color.red = 0x00;
        color.alpha = 0xFF;
      }
    }
    else if (layerColorIdx < paletteData_.num_palette_entries)
      color = palette_[layerColorIdx];
    else
      continue;

    if (FT_Bitmap_Blend(library_,
                        &bitmapGlyph->bitmap, slotOffset,
                        &bitmap, &bitmapOffset,
                        color))
    {
      // XXX error
      failed = true;
      break;
    }
  } while (FT_Get_Color_Glyph_Layer(ftSize_->face,
                                    glyphIndex,
                                    &layerGlyphIdx,
                                    &layerColorIdx,
                                    &iter));

  imageType_.flags = oldLoadFlags;
  if (failed)
  {
    FT_Bitmap_Done(library_, &bitmap);
    return NULL;
  }

  auto img = convertBitmapToQImage(&bitmap);
  if (outRect)
  {
    outRect->setSize(img->size());
    outRect->setLeft(bitmapOffset.x >> 6);
    outRect->setTop(bitmapOffset.y >> 6);
  }

  FT_Bitmap_Done(library_, &bitmap);

  return img;
}


QHash<FT_Glyph_Format, QString> glyphFormatNamesCache;
QHash<FT_Glyph_Format, QString>&
glyphFormatNames()
{
  if (glyphFormatNamesCache.empty())
  {
    glyphFormatNamesCache[FT_GLYPH_FORMAT_NONE] = "None/Unknown";
    glyphFormatNamesCache[FT_GLYPH_FORMAT_COMPOSITE] = "Composite";
    glyphFormatNamesCache[FT_GLYPH_FORMAT_BITMAP] = "Bitmap";
    glyphFormatNamesCache[FT_GLYPH_FORMAT_OUTLINE] = "Outline";
    glyphFormatNamesCache[FT_GLYPH_FORMAT_PLOTTER] = "Plotter";
#if FREETYPE_MINOR >= 12
    glyphFormatNamesCache[FT_GLYPH_FORMAT_SVG] = "SVG";
#endif
  }
  return glyphFormatNamesCache;
}

QString*
glyphFormatToName(FT_Glyph_Format format)
{
  auto& names = glyphFormatNames();
  auto it = names.find(format);
  if (it == names.end())
    return &names[FT_GLYPH_FORMAT_NONE];
  return &it.value();
}


// end of engine.cpp
