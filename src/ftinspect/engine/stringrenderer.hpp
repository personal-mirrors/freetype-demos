// stringrenderer.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once

#include <functional>
#include <vector>

#include <QString>
#include <qslider.h>

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftcache.h>
#include <freetype/ftglyph.h>

// adopted from `ftcommon.h`

class Engine;

struct GlyphContext
{
  int charCode = 0;
  int charCodeUcs4 = 0;
  int glyphIndex = 0;
  FT_Glyph glyph = NULL;
  FTC_Node cacheNode = NULL;

  FT_Pos lsbDelta = 0; // Delta caused by hinting.
  FT_Pos rsbDelta = 0; // Delta caused by hinting.

  FT_Vector hadvance = { 0, 0 }; // Kerned horizontal advance.
  FT_Vector vvector = { 0, 0 };  // Vertical origin to horizontal origin.
  FT_Vector vadvance = { 0, 0 }; // Vertical advance.
};


// Class to populate characters to render, to load and properly position
// glyphs.  Use callbacks to receive characters and lines.  You should save
// the result from the callbacks to a cache.
class StringRenderer
{
public:
  StringRenderer(Engine* engine);
  ~StringRenderer();

  enum KerningDegree // XXX: Not honored actually
  {
    KD_None = 0,
    KD_Light,
    KD_Medium,
    KD_Tight
  };

  enum KerningMode
  {
    KM_None = 0,
    KM_Normal,
    KM_Smart
  };

  // Called when outputting a glyph.  The receiver is reponsible for
  // rendering the glyph to bitmap.
  //
  // We need to pass the pen position because sometimes the outline vector
  // contains no points and thus can't be translated to the desired pen
  // position.
  using RenderCallback = std::function<void(FT_Glyph, // glyph
                                            FT_Vector, // penPos
                                            GlyphContext&)>;

  // Called when outputting a glyph with a pre-rendered bitmap.  The
  // receiver can simply use the bitmap, mainly for color layered fonts.
  //
  // TODO: Remove `RenderCallback` and do QImage creation in this class?
  //
  // The receiver is responsible for deleteing the `QImage` (ownership
  // transfered).
  using RenderImageCallback = std::function<void(QImage*, // bitmap
                                                 QRect, // bbox
                                                 FT_Vector, // penPos
                                                 FT_Vector, // advance
                                                 GlyphContext&)>;

  // Called right after the glyph is obtained from the font, before any
  // other operation is done.  The receiver can do pre-processing like
  // slanting and emboldening in this function.
  //
  // The glyph pointer may be replaced.  In that case, ownership is
  // transfered to the renderer, and the new glyph will be eventually freed
  // by the renderer.  The callback is responsible to free the old glyph.
  // This allows you to do the following:
  //
  //   void callback(FT_Glyph* ptr)
  //   {
  //     ...
  //     auto oldPtr = *ptr;
  //     *ptr = ...;
  //     FT_Done_Glyph(oldPtr);
  //   }
  using PreprocessCallback = std::function<void(FT_Glyph*)>;

  // Called when a new line begins.
  using LineBeginCallback = std::function<void(FT_Vector, // initial penPos
                                               double)>; // size (points)

  //////// Getters
  bool isWaterfall() { return waterfall_; }
  double position(){ return position_; }
  int charMapIndex() { return charMapIndex_; }

  //////// Callbacks
  void setCallback(RenderCallback cb)
         { renderCallback_ = std::move(cb); }
  void setImageCallback(RenderImageCallback cb)
         { renderImageCallback_ = std::move(cb); }
  void setPreprocessCallback(PreprocessCallback cb)
         { glyphPreprocessCallback_ = std::move(cb); }
  void setLineBeginCallback(LineBeginCallback cb)
         { lineBeginCallback_ = std::move(cb); }

  //////// Setters for options
  void setCharMapIndex(int charMapIndex,
                       int limitIndex);
  void setRepeated(bool repeated) { repeated_ = repeated; }
  void setVertical(bool vertical) { vertical_ = vertical; }
  void setRotation(double rotation);
  void setWaterfall(bool waterfall) { waterfall_ = waterfall; }
  void setWaterfallParameters(double start,
                              double end)
  {
    waterfallStart_ = start;
    waterfallEnd_ = end;
  }
  void setPosition(double pos) { position_ = pos; }
  void setLsbRsbDelta(bool enabled) { lsbRsbDeltaEnabled_ = enabled; }
  void setKerning(bool kerning);

  // Need to be called when font or charMap changes.
  void setUseString(QString const& string);
  void setUseAllGlyphs();

  //////// Actions
  int render(int width,
             int height,
             int offset);
  int renderLine(int x,
                 int y,
                 int width,
                 int height,
                 int offset,
                 bool handleMultiLine = false);

  void reloadAll(); // Text/font/charmap changes, will call `reloadGlyphs`.
  void reloadGlyphs(); // Any other parameter changes.

private:
  Engine* engine_;

  // Generally, rendering has those steps:
  //
  // 1. If in string mode, the string is load into `activeGlyphs_`
  //    (in `updateString`).
  // 2. The character codes in contexts are converted to glyph indices
  //    (in `reloadGlyphIndices`).
  // 3. If in string mode, glyphs are loaded into contexts
  //    (in `loadStringGlyphs`).
  // 4. In `render` function, according to mode, `renderLine` is called line
  //    by line (as well as `prepareRendering`).
  // 5. In `renderLine`, if in all glyphs mode, glyphs from the begin index
  //    are loaded until the line is full (if the glyph already exists, it
  //    will be reused).  If in string mode, it will directly use the
  //    prepared glyphs.  Preprocessing is done within this step, such as
  //    emboldening or stroking.  Eventually the `FT_Glyph` pointer is
  //    passed to the callback.

  GlyphContext tempGlyphContext_;

  // This vector stores all active glyphs for rendering.  When rendering
  // strings, this is the container for characters, so DO NOT directly clear
  // it to flush the cache!  You should clean glyph objects only.  However,
  // when rendering all glyphs, it's generally better to directly wipe the
  // vector because it is dynamically generated in `render` function (see
  // above).
  //
  // Note: Because of kerning, this list must be ordered and allow duplicate
  //       characters.
  //
  // Actually this means 3 parts of storage: string character code, glyph
  // indices, and glyph (+ all related info).  Different parameter changes
  // will trigger different levels of flushing.
  std::vector<GlyphContext> activeGlyphs_;
  bool glyphCacheValid_ = false;

  int charMapIndex_ = 0;
  int limitIndex_ = 0;
  bool usingString_ = false;
  bool repeated_ = false;
  bool vertical_ = false;
  double position_ = 0.5;
  double rotation_ = 0;
  int kerningDegree_ = KD_None;
  KerningMode kerningMode_ = KM_None;
  FT_Pos trackingKerning_ = 0;
  FT_Matrix matrix_ = {};
  bool matrixEnabled_ = false;
  bool lsbRsbDeltaEnabled_ = true;

  bool waterfall_ = false;
  double waterfallStart_ = -1;
  double waterfallEnd_ = -1; // -1 = Auto

  RenderCallback renderCallback_;
  RenderImageCallback renderImageCallback_;
  PreprocessCallback glyphPreprocessCallback_;
  LineBeginCallback lineBeginCallback_;

  void reloadGlyphIndices(); // For string rendering.
  void prepareRendering();
  void loadSingleContext(GlyphContext* ctx,
                         GlyphContext* prev);
  // Need to be called when font, charMap or size changes.
  void loadStringGlyphs();
  // Returns total line count.
  int prepareLine(int offset,
                  int lineWidth,
                  FT_Vector& outActualLineWidth,
                  int nonSpacingPlaceholder,
                  bool handleMultiLine = false);
  void clearActive(bool glyphOnly = false);

  int convertCharEncoding(int charUcs4,
                          FT_Encoding encoding);
};


// end of stringrenderer.hpp
