// glyphcontinuous.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include "graphicsdefault.hpp"
#include "../engine/stringrenderer.hpp"

#include <utility>
#include <vector>

#include <QWidget>
#include <QImage>

#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/ftstroke.h>


struct GlyphCacheEntry
{
  QImage* image = NULL;
  QRect basePosition = {};
  QPoint penPos = {};
  int charCode = -1;
  int glyphIndex = -1;

  FT_Vector advance = {};
  bool hasAdvance = false;

  GlyphCacheEntry() {}
  ~GlyphCacheEntry();
  GlyphCacheEntry(const GlyphCacheEntry& other) = delete;
  GlyphCacheEntry& operator=(const GlyphCacheEntry& other) = delete;
  GlyphCacheEntry(GlyphCacheEntry&& other) noexcept;
  GlyphCacheEntry& operator=(GlyphCacheEntry&& other) noexcept;
};


struct GlyphCacheLine
{
  QPoint basePosition = {};
  double sizePoint = 0.0;
  std::vector<GlyphCacheEntry> entries;
};


class Engine;
class GlyphContinuous
: public QWidget
{
  Q_OBJECT
public:
  GlyphContinuous(QWidget* parent, Engine* engine);
  ~GlyphContinuous() override;

  enum Source : int
  {
    SRC_AllGlyphs,
    SRC_TextString,
    SRC_TextStringRepeated
  };

  enum Mode : int
  {
    M_Normal,
    M_Fancy,
    M_Stroked
  };

  int displayingCount() { return displayingCount_; }
  StringRenderer& stringRenderer() { return stringRenderer_; }

  // all those setters don't trigger repaint.
  void setBeginIndex(int index) { beginIndex_ = index; }
  void setSource(Source source);
  void setMode(Mode mode) { mode_ = mode; }
  void setFancyParams(double boldX, double boldY, double slant)
  {
    boldX_ = boldX;
    boldY_ = boldY;
    slant_ = slant;
  }
  void setStrokeRadius(double radius) { strokeRadius_ = radius; }
  void setSourceText(QString text);

  void purgeCache();

signals:
  void wheelNavigate(int steps);
  void wheelResize(int steps);
  void displayingCountUpdated(int newCount);

protected:
  void paintEvent(QPaintEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

private:
  Engine* engine_;
  StringRenderer stringRenderer_;

  Source source_ = SRC_AllGlyphs;
  Mode mode_ = M_Normal;
  int beginIndex_;
  double boldX_, boldY_, slant_;
  double strokeRadius_;
  QString text_;
  int sizeIndicatorOffset_; // For Waterfall Rendering...

  int displayingCount_ = 0;
  FT_Size_Metrics metrics_;
  int x_ = 0, y_ = 0;
  int stepY_ = 0;
  FT_Pos emboldeningX_, emboldeningY_;
  FT_Matrix shearMatrix_;

  FT_Stroker stroker_;

  std::vector<GlyphCacheLine> glyphCache_;
  GlyphCacheLine* currentWritingLine_ = NULL;

  QPoint positionDelta_;

  void paintByRenderer();

  // These two are used indendpent of current glyph variables
  // and assumes ownership of glyphs, but don't free them.
  // However, remember to free the glyph returned from `transformGlyphStroked`
  void transformGlyphFancy(FT_Glyph glyph);
  FT_Glyph transformGlyphStroked(FT_Glyph glyph);

  void paintCache(QPainter* painter);
  void fillCache();
  void prePaint();
  void updateRendererText();
  void preprocessGlyph(FT_Glyph* glyphPtr);
  void beginSaveLine(FT_Vector pos,
                     double sizePoint);
  void saveSingleGlyph(FT_Glyph glyph,
                       FT_Vector penPos,
                       GlyphContext gctx);
  void saveSingleGlyphImage(QImage* image,
                            QRect pos,
                            GlyphContext gctx);
  void beginDrawCacheLine(QPainter* painter,
                          const GlyphCacheLine& line);
  void drawCacheGlyph(QPainter* painter,
                      const GlyphCacheEntry& entry);
};


// end of glyphcontinuous.hpp
