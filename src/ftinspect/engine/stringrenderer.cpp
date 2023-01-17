// stringrenderer.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "engine.hpp"
#include "stringrenderer.hpp"

#include <cmath>

#include <QTextCodec>


StringRenderer::StringRenderer(Engine* engine)
: engine_(engine)
{
}


StringRenderer::~StringRenderer()
{
  clearActive();
}


void
StringRenderer::setCharMapIndex(int charMapIndex,
                                int limitIndex)
{
  auto& charMaps = engine_->currentFontCharMaps();
  if (charMapIndex < 0
      || static_cast<unsigned>(charMapIndex) >= charMaps.size())
    charMapIndex = -1;

  charMapIndex_ = charMapIndex;
  limitIndex_ = limitIndex;
}


void
StringRenderer::setRotation(double rotation)
{
  rotation_ = rotation;

  if (rotation <= -180)
    rotation += 360;
  if (rotation > 180)
    rotation -= 360;

  if (rotation == 0)
  {
    matrixEnabled_ = false;
    return;
  }

  matrixEnabled_ = true;
  double radian = rotation * 3.14159265 / 180.0;
  auto cosinus = static_cast<FT_Fixed>(cos(radian) * 65536.0);
  auto sinus = static_cast<FT_Fixed>(sin(radian) * 65536.0);

  matrix_.xx = cosinus;
  matrix_.yx = sinus;
  matrix_.xy = -sinus;
  matrix_.yy = cosinus;
}


void
StringRenderer::setKerning(bool kerning)
{
  if (kerning)
  {
    kerningMode_ = KM_Smart;
    kerningDegree_ = KD_Medium;
  }
  else
  {
    kerningMode_ = KM_None;
    kerningDegree_ = KD_None;
  }
}


void
StringRenderer::reloadAll()
{
  clearActive(usingString_); // If 'All Glyphs', do a complete wipe.
  if (usingString_)
    reloadGlyphIndices();
}


void
StringRenderer::reloadGlyphs()
{
  clearActive(true);
}


void
StringRenderer::setUseString(QString const& string)
{
  clearActive(); // Clear existing data.
  usingString_ = true;

  long long totalCount = 0;
  for (uint ch : string.toUcs4())
  {
    activeGlyphs_.emplace_back();
    auto& it = activeGlyphs_.back();
    it.charCodeUcs4 = it.charCode = static_cast<int>(ch);
    it.glyphIndex = 0;
    ++totalCount;
    if (totalCount >= INT_MAX) // Prevent overflow.
      break;
  }
  reloadGlyphIndices();
}


void
StringRenderer::setUseAllGlyphs()
{
  if (usingString_)
    clearActive();
  usingString_ = false;
}


void
StringRenderer::reloadGlyphIndices()
{
  if (!usingString_)
    return;
  int charMapIndex = charMapIndex_;
  auto& charMaps = engine_->currentFontCharMaps();
  if (charMaps.empty())
    return;
  if (charMapIndex < 0
      || static_cast<unsigned>(charMapIndex) >= charMaps.size())
    charMapIndex = engine_->currentFontFirstUnicodeCharMap();
  if (charMapIndex < 0
      || static_cast<unsigned>(charMapIndex) >= charMaps.size())
    charMapIndex = 0;
  auto encoding = charMaps[charMapIndex].encoding;

  if (charMapIndex < 0)
    return;
  for (auto& ctx : activeGlyphs_)
  {
    if (encoding != FT_ENCODING_UNICODE)
      ctx.charCode = convertCharEncoding(ctx.charCodeUcs4, encoding);

    auto index = engine_->glyphIndexFromCharCode(ctx.charCode, charMapIndex);
    ctx.glyphIndex = static_cast<int>(index);
  }
}


void
StringRenderer::prepareRendering()
{
  engine_->reloadFont();
  if (!engine_->renderReady())
    return;
  engine_->loadPalette();
  if (kerningDegree_ != KD_None)
    trackingKerning_ = engine_->currentFontTrackingKerning(kerningDegree_);
  else
    trackingKerning_ = 0;
}


void
StringRenderer::loadSingleContext(GlyphContext* ctx,
                                  GlyphContext* prev)
{
  if (ctx->cacheNode)
  {
    FTC_Node_Unref(ctx->cacheNode, engine_->cacheManager());
    ctx->cacheNode = NULL;
  }
  else if (ctx->glyph)
    FT_Done_Glyph(ctx->glyph); // When caching isn't used.

  // TODO use FTC?

  // After `prepareRendering`, current size/face is properly set.
  FT_GlyphSlot slot = engine_->currentFaceSlot();
  if (engine_->loadGlyphIntoSlotWithoutCache(ctx->glyphIndex) != 0)
  {
    ctx->glyph = NULL;
    return;
  }
  if (FT_Get_Glyph(slot, &ctx->glyph) != 0)
  {
    ctx->glyph = NULL;
    return;
  }
  auto& metrics = slot->metrics;
  //ctx->glyph = engine_->loadGlyphWithoutUpdate(ctx->glyphIndex,
  //                                             &ctx->cacheNode);

  if (!ctx->glyph)
    return;

  ctx->vvector.x = metrics.vertBearingX - metrics.horiBearingX;
  ctx->vvector.y = -metrics.vertBearingY - metrics.horiBearingY;

  ctx->vadvance.x = 0;
  ctx->vadvance.y = -metrics.vertAdvance;

  ctx->lsbDelta = slot->lsb_delta;
  ctx->rsbDelta = slot->rsb_delta;

  ctx->hadvance.x = metrics.horiAdvance;
  ctx->hadvance.y = 0;

  if (lsbRsbDeltaEnabled_ && engine_->lcdUsingSubPixelPositioning())
    ctx->hadvance.x += ctx->lsbDelta - ctx->rsbDelta;
  prev->hadvance.x += trackingKerning_;

  if (kerningMode_ != KM_None)
  {
    FT_Vector kern = engine_->currentFontKerning(ctx->glyphIndex,
                                                 prev->glyphIndex);

    prev->hadvance.x += kern.x;
    prev->hadvance.y += kern.y;
  }

  if (!engine_->lcdUsingSubPixelPositioning()
      && lsbRsbDeltaEnabled_)
  {
    if (prev->rsbDelta - ctx->lsbDelta > 32)
      prev->hadvance.x -= 64;
    else if (prev->rsbDelta - ctx->lsbDelta < -31)
      prev->hadvance.x += 64;
  }

  if (!engine_->lcdUsingSubPixelPositioning() && engine_->doHinting())
  {
    prev->hadvance.x = (prev->hadvance.x + 32) & -64;
    prev->hadvance.y = (prev->hadvance.y + 32) & -64;
  }
}


void
StringRenderer::loadStringGlyphs()
{
  if (!usingString_)
    return;
  GlyphContext* prev = &tempGlyphContext_; // = empty
  tempGlyphContext_ = {};

  for (auto& ctx : activeGlyphs_)
  {
    loadSingleContext(&ctx, prev);
    prev = &ctx;
  }

  glyphCacheValid_ = true;
}


int
StringRenderer::prepareLine(int offset,
                            int lineWidth,
                            FT_Vector& outActualLineWidth,
                            int nonSpacingPlaceholder,
                            bool handleMultiLine)
{
  int totalCount = 0;
  outActualLineWidth = {0, 0};
  if (!usingString_) // All glyphs
  {
    // The situation gets a little complicated when we are using the 'All
    // Glyphs' mode: The input sequence is actually infinite so we have to
    // combine loading glyph into rendering and can't preload all glyphs.

    // TODO: Low performance when the begin index is large.
    // TODO: Optimize: use a sparse vector...!
    // The problem is that when doing a `list::resize`, the ctor is called
    // unnecessarily often.
    tempGlyphContext_ = {};
    for (unsigned n = offset; n < static_cast<unsigned>(limitIndex_);)
    {
      if (activeGlyphs_.capacity() <= n)
        activeGlyphs_.reserve(static_cast<size_t>(n) * 2);
      if (activeGlyphs_.size() <= n)
        activeGlyphs_.resize(n + 1);

      auto& ctx = activeGlyphs_[n];
      ctx.charCode = static_cast<int>(n);
      ctx.glyphIndex = static_cast<int>(
                         engine_->glyphIndexFromCharCode(static_cast<int>(n),
                                                         charMapIndex_));

      auto prev = n == 0 ? &tempGlyphContext_ : &activeGlyphs_[n - 1];
      if (!ctx.glyph)
        loadSingleContext(&ctx, prev);

      // In 'All Glyphs' mode, a red placeholder should be drawn for
      // non-spacing glyphs (e.g., the stress mark).
      auto actualAdvanceX = ctx.hadvance.x ? ctx.hadvance.x
                                           : nonSpacingPlaceholder << 6;
      if (outActualLineWidth.x + actualAdvanceX > lineWidth)
        break;
      outActualLineWidth.x += actualAdvanceX;
      outActualLineWidth.y += ctx.hadvance.y;
      ++n;
      ++totalCount;
    }
  }
  else // strings
  {
    if (!glyphCacheValid_)
    {
      clearActive(true);
      loadStringGlyphs();
    }

    for (unsigned n = offset; n < activeGlyphs_.size();)
    {
      auto& ctx = activeGlyphs_[n];

      if (handleMultiLine && ctx.charCode == '\n')
      {
        totalCount += 1; // Break here.
        break;
      }

      if (repeated_) // If repeated, we must stop when we touch
                     // the end of the line
      {
        if (outActualLineWidth.x + ctx.hadvance.x > lineWidth)
          break;
        outActualLineWidth.x += ctx.hadvance.x;
        outActualLineWidth.y += ctx.hadvance.y;
        ++n;
        n %= static_cast<int>(activeGlyphs_.size()); // safe
      }
      else if (vertical_)
      {
        outActualLineWidth.x += ctx.vadvance.x;
        outActualLineWidth.y += ctx.vadvance.y;
        ++n;
      }
      else
      {
        outActualLineWidth.x += ctx.hadvance.x;
        outActualLineWidth.y += ctx.hadvance.y;
        ++n;
      }
      ++totalCount;
    }
  }
  return totalCount;
}


int
StringRenderer::render(int width,
                       int height,
                       int offset)
{
  engine_->reloadFont();

  if (usingString_)
    offset = 0;
  if (!usingString_ && limitIndex_ <= 0)
    return 0;
  if (!engine_->fontValid())
    return 0;

  auto initialOffset = offset;

  // Separate into 3 modes: Waterfall, fill the whole canvas, and render a
  // single string only.
  if (waterfall_)
  {
    // Waterfall

    vertical_ = false;
    // They are only effective for non-bitmap-only (scalable) fonts!
    auto originalSize = static_cast<int>(engine_->pointSize() * 64);
    auto ptSize = originalSize;
    auto ptHeight = 64 * 72 * height / engine_->dpi();
    int step = 0;

    auto bitmapOnly = engine_->currentFontBitmapOnly();
    auto fixedSizes = engine_->currentFontFixedSizes();
    std::sort(fixedSizes.begin(), fixedSizes.end());
    auto fixedSizesIter = fixedSizes.begin();

    if (waterfallStart_ <= 0)
    {
      // auto
      step = (originalSize * originalSize / ptHeight + 64) & ~63;
      ptSize = ptSize - step * (ptSize / step); // modulo
      ptSize += step;
    }
    else if (!bitmapOnly)
    {
      ptSize = static_cast<int>(waterfallStart_ * 64.0) & ~31;
      // We first get a ratio since height & ppem are near proportional...
      // Value 64.0 is somewhat a magic reference number.
      engine_->setSizeByPoint(64.0);
      engine_->reloadFont();
      if (!engine_->renderReady())
        return -1;
      auto pixelActual = engine_->currentFontMetrics().height >> 6;

      auto heightPt = height * 64.0 / pixelActual;

      if (waterfallEnd_ < waterfallStart_)
        waterfallEnd_ = waterfallStart_ + 1;

      auto n = heightPt * 2 / (waterfallStart_ + waterfallEnd_);
      auto stepTemp = (waterfallEnd_ - waterfallStart_) / (n + 1);
      // Rounding to 0.25.
      step = static_cast<int>(std::round(stepTemp * 4)) * 16 & ~15;
      if (step == 0)
        step = 16; // 0.25pt
    }

    int y = 0;
    // No position parameter in 'All Glyphs' or repeated mode.
    int x = static_cast<int>((usingString_ && !repeated_)
              ? (width * position_)
              : 0);
    int count = 0;

    while (true)
    {
      if (!bitmapOnly)
        engine_->setSizeByPoint(ptSize / 64.0);
      else
      {
        if (fixedSizesIter == fixedSizes.end())
          break;
        engine_->setSizeByPixel(*fixedSizesIter);
      }
      clearActive(true);
      prepareRendering(); // Set size/face for engine to have valid metrics.
      auto& metrics = engine_->currentFontMetrics();

      y += static_cast<int>(metrics.height >> 6) + 1;
      if (y >= height && !bitmapOnly)
        break;

      loadStringGlyphs();
      auto lcount = renderLine(x,
                               y + static_cast<int>(metrics.descender >> 6),
                               width,
                               height,
                               offset);
      count = std::max(count, lcount);

      if (!bitmapOnly)
      {
        if (step == 0)
          break;
        ptSize += step;
      }
      else
        ++fixedSizesIter;
    }
    engine_->setSizeByPoint(originalSize / 64.0);

    return count;
  }
  // end of waterfall

  if (repeated_ || !usingString_)
  {
    // Fill the whole canvas (string repeated or all glyphs).

    prepareRendering();
    if (!engine_->renderReady())
      return 0;
    auto& metrics = engine_->currentFontMetrics();
    auto y = 4 + static_cast<int>(metrics.ascender >> 6);
    auto stepY = static_cast<int>(metrics.height >> 6) + 1;
    auto limitY = height + static_cast<int>(metrics.descender >> 6);

    // Only care about multi-line rendering when in string mode.
    for (; y < limitY; y += stepY)
    {
      offset = renderLine(0, y, width, height, offset, usingString_);
      // For repeating.
      if (usingString_ && repeated_ && !activeGlyphs_.empty())
        offset %= static_cast<int>(activeGlyphs_.size());
    }
    if (!usingString_) // Only return count for 'All Glyphs' mode.
      return offset - initialOffset;
    return 0;
  }

  // single string
  prepareRendering();
  if (!engine_->renderReady())
    return 0;

  auto& metrics = engine_->currentFontMetrics();
  auto x = static_cast<int>(width * position_);
  // Anchor at top-left in vertical mode, at the center in horizontal mode.
  auto y = vertical_ ? 0 : (height / 2);
  auto stepY = static_cast<int>(metrics.height >> 6) + 1;
  y += 4 + static_cast<int>(metrics.ascender >> 6);

  auto lastOffset = 0;
  while (offset < static_cast<int>(activeGlyphs_.size()))
  {
    offset = renderLine(x, y, width, height, offset, true);
    if (offset == lastOffset) // Prevent infinite loop.
      break;
    lastOffset = offset;
    y += stepY;
  }
  return offset - initialOffset;
}


int
StringRenderer::renderLine(int x,
                           int y,
                           int width,
                           int height,
                           int offset,
                           bool handleMultiLine)
{
  // Don't limit (x, y) to be within the canvas viewport: the string can be
  // moved by the mouse.

  y = height - y; // Change to Cartesian coordinates.

  FT_Vector pen = { 0, 0 };
  FT_Vector advance;
  auto nonSpacingPlaceholder = engine_->currentFontMetrics().y_ppem / 2 + 2;

  // When in 'All Glyphs' mode, no vertical support.
  if (repeated_ || !usingString_)
    vertical_ = false; // TODO: Support vertical + repeated

  int lineLength = 64 * (vertical_ ? height : width);

  // First prepare the line & determine the line length.
  int totalCount = prepareLine(offset, lineLength, pen,
                               nonSpacingPlaceholder, handleMultiLine);

  // Round to control initial pen position and preserve hinting...
  // (pen.x, y) is the actual length now, and we multiple it by position.
  auto centerFixed = static_cast<int>(0x10000 * position_);
  if (!usingString_ || repeated_)
    centerFixed = 0;
  pen.x = FT_MulFix(pen.x, centerFixed) & ~63;
  pen.y = FT_MulFix(pen.y, centerFixed) & ~63;

  // ... unless rotating; XXX sbits
  if (matrixEnabled_)
    FT_Vector_Transform(&pen, &matrix_);

  // get pen position: penPos = center - pos * width
  pen.x = (x << 6) - pen.x;
  pen.y = (y << 6) - pen.y;

  // Need to transform the coordinates back to normal coordinate system.
  lineBeginCallback_({ (pen.x >> 6),
                       height - (pen.y >> 6) },
                     engine_->pointSize());

  for (int i = offset; i < totalCount + offset; i++)
  {
    auto& ctx = activeGlyphs_[i % activeGlyphs_.size()];
    if (handleMultiLine && ctx.charCode == '\n')
      continue; // Skip \n.
    FT_Glyph image = NULL; // Remember to clean up.
    FT_BBox bbox;

    if (!ctx.glyph)
      continue;

    advance = vertical_ ? ctx.vadvance : ctx.hadvance;

    QRect rect;
    QImage* colorLayerImage
      = engine_->renderingEngine()->tryDirectRenderColorLayers(ctx.glyphIndex,
                                                               &rect,
                                                               true);

    if (colorLayerImage)
    {
      FT_Vector penPos = { (pen.x >> 6), height - (pen.y >> 6) };
      renderImageCallback_(colorLayerImage, rect, penPos, advance, ctx);
    }
    else
    {
      // Copy the glyph because we're doing manipulation.
      auto error = FT_Glyph_Copy(ctx.glyph, &image);
      if (error)
        continue;

      glyphPreprocessCallback_(&image);

      if (image->format != FT_GLYPH_FORMAT_BITMAP)
      {
        if (vertical_)
          error = FT_Glyph_Transform(image, NULL, &ctx.vvector);

        if (!error)
        {
          if (matrixEnabled_)
            error = FT_Glyph_Transform(image, &matrix_, NULL);
        }

        if (error)
        {
          FT_Done_Glyph(image);
          continue;
        }
      }
      else
      {
        auto bitmap = reinterpret_cast<FT_BitmapGlyph>(image);

        if (vertical_)
        {
           bitmap->left += static_cast<int>(ctx.vvector.x) >> 6;
           bitmap->top += static_cast<int>(ctx.vvector.y) >> 6;
        }
      }

      if (matrixEnabled_)
        FT_Vector_Transform(&advance, &matrix_);

      FT_Glyph_Get_CBox(image, FT_GLYPH_BBOX_PIXELS, &bbox);

      // Don't check for bounding box here.
      FT_Vector penPos = { (pen.x >> 6), height - (pen.y >> 6) };
      renderCallback_(image, penPos, ctx);

      FT_Done_Glyph(image);
    }

    pen.x += advance.x;
    pen.y += advance.y;

    if (!advance.x && !usingString_) // Add placeholder.
      pen.x += nonSpacingPlaceholder << 6;
  }

  return offset + totalCount;
}


void
StringRenderer::clearActive(bool glyphOnly)
{
  for (auto& ctx : activeGlyphs_)
  {
    if (ctx.cacheNode)
      FTC_Node_Unref(ctx.cacheNode, engine_->cacheManager());
    else if (ctx.glyph)
      FT_Done_Glyph(ctx.glyph); // When caching isn't used.
    ctx.cacheNode = NULL;
    ctx.glyph = NULL;
  }
  if (!glyphOnly)
    activeGlyphs_.clear();

  glyphCacheValid_ = false;
}


int
StringRenderer::convertCharEncoding(int charUcs4,
                                    FT_Encoding encoding)
{
  switch (encoding)
  {
  case FT_ENCODING_MS_SYMBOL:
  case FT_ENCODING_UNICODE:
  case FT_ENCODING_ADOBE_STANDARD: // These may be problematic...
  case FT_ENCODING_ADOBE_EXPERT:
  case FT_ENCODING_ADOBE_CUSTOM:
  case FT_ENCODING_ADOBE_LATIN_1:
    return charUcs4;
  default:
    ; // Proceed.
  }

  auto mib = -1;
  switch (encoding)
  {
  case FT_ENCODING_SJIS:
    mib = 17; // Shift_JIS
    break;
  case FT_ENCODING_PRC:
    mib = 114; // GB 18030
    break;
  case FT_ENCODING_BIG5:
    mib = 2026; // Big5
    break;
  case FT_ENCODING_WANSUNG:
    mib = -949; // KS C 5601:1987, this is a fake mib value.
    break;
  case FT_ENCODING_JOHAB:
    mib = 38; //  KS C 5601:1992 / EUC-KR
    break;
  case FT_ENCODING_APPLE_ROMAN:
    mib = 2027;
    break;
  default:
    return charUcs4; // Failed.
  }

  if (mib == -1)
    return charUcs4; // Unsupported charmap.
  auto codec = QTextCodec::codecForMib(mib);
  if (!codec)
    return charUcs4; // Unsupported.

  auto res = codec->fromUnicode(
               QString::fromUcs4(reinterpret_cast<uint*>(&charUcs4), 1));
  if (res.size() == 0)
    return charUcs4;
  if (res.size() == 1)
    return res[0];
  return ((static_cast<int>(res[0]) & 0xFF) << 8)
         | (static_cast<int>(res[1]) & 0xFF);
}


// end of stringrenderer.cpp
