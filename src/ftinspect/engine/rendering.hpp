// rendering.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include <QColor>
#include <QImage>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>

class Engine;
class RenderingEngine
{
public:
  RenderingEngine(Engine* engine);

  void setForeground(QRgb foreground);
  void setBackground(QRgb background);
  void setGamma(double gamma);
  void calculateForegroundTable();
  void setLCDUsesBGR(bool isBGR) { lcdUsesBGR_ = isBGR; }

  QRgb foreground() { return foregroundColor_; }
  QRgb background() { return backgroundColor_; }
  double gamma() { return gamma_; }

  // Return `true` if you need to free `out`
  // `out` will be set to NULL in cases of error
  bool convertGlyphToBitmapGlyph(FT_Glyph src, FT_Glyph* out);
  FT_Bitmap convertBitmapTo8Bpp(FT_Bitmap* bitmap);
  QImage* convertBitmapToQImage(FT_Bitmap* src);
  QImage* convertGlyphToQImage(FT_Glyph src, 
                               QRect* outRect,
                               bool inverseRectY);
  QPoint computeGlyphOffset(FT_Glyph glyph, bool inverseY);

  /*
   * Directly render the glyph at the specified index
   * to a `QImage`. If you want to perform color-layer
   * rendering, call this before trying to load the
   * glyph and do normal rendering, If the returning
   * value is non-NULL, then there's no need to
   * load the glyph the normal way, just draw the `QImage`.
   * Will return NULL if not enabled or color layers not available.
   */
  QImage* tryDirectRenderColorLayers(int glyphIndex,
                                     QRect* outRect,
                                     bool inverseRectY = false);

  QPixmap padToSize(QImage* image, int ppem);

private:
  Engine* engine_;

  QRgb backgroundColor_;
  QRgb foregroundColor_;
  double gamma_;
  QVector<QRgb> foregroundTable_;

  bool lcdUsesBGR_;
};


// end of rendering.hpp
