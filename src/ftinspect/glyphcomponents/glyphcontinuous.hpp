// glyphcontinuous.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once

#include "../engine/stringrenderer.hpp"
#include "graphicsdefault.hpp"

#include <utility>
#include <vector>

#include <QImage>
#include <QTimer>
#include <QWidget>

#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/ftstroke.h>


// We store images in the cache so we don't need to render all glyphs every time
// when repainting the widget.
struct GlyphCacheEntry
{
  QImage* image = NULL;
  QRect basePosition = {};
  QPoint penPos = {};
  int charCode = -1;
  int glyphIndex = -1;
  unsigned nonSpacingPlaceholder = 0;

  FT_Vector advance = {};

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
  int sizeIndicatorOffset;
  unsigned short nonSpacingPlaceholder;
  std::vector<GlyphCacheEntry> entries;
};


class Engine;

class GlyphContinuous
: public QWidget
{
  Q_OBJECT
public:
  GlyphContinuous(QWidget* parent,
                  Engine* engine);
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

  // All those setters don't trigger a repaint operation.
  void setBeginIndex(int index) { beginIndex_ = index; }
  void setSource(Source source);
  void setMode(Mode mode) { mode_ = mode; }
  void setScale(double scale) { scale_ = scale; }
  void setFancyParams(double boldX,
                      double boldY,
                      double slant)
  {
    boldX_ = boldX;
    boldY_ = boldY;
    slant_ = slant;
  }
  void setStrokeRadius(double radius) { strokeRadius_ = radius; }
  void setSourceText(QString text);
  void setMouseOperationEnabled(bool enabled)
         { mouseOperationEnabled_ = enabled; }

  void flashOnGlyph(int glyphIndex);
  void stopFlashing();
  void purgeCache();
  void resetPositionDelta();

signals:
  void wheelNavigate(int steps);
  void wheelResize(int steps);
  void wheelZoom(int steps);
  void beginIndexChangeRequest(int newIndex);
  void displayingCountUpdated(int newCount);
  void rightClickGlyph(int glyphIndex,
                       double sizePoint);
  void updateGlyphDetails(GlyphCacheEntry* ctxt,
                          int charMapIndex,
                          bool open);

protected:
  void paintEvent(QPaintEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

private:
  Engine* engine_;
  StringRenderer stringRenderer_;

  QTimer* flashTimer_;
  int flashRemainingCount_ = 0;
  int flashGlyphIndex_ = -1;

  Source source_ = SRC_AllGlyphs;
  Mode mode_ = M_Normal;
  int beginIndex_;
  double boldX_;
  double boldY_;
  double slant_;
  double strokeRadius_;
  QString text_;
  int sizeIndicatorOffset_ = 0; // For Waterfall Rendering...

  bool mouseOperationEnabled_ = true;
  int displayingCount_ = 0;
  FT_Fixed strokeRadiusForSize_ = 0;
  double scale_ = 1.0;
  FT_Matrix shearMatrix_;

  FT_Stroker stroker_;

  std::vector<GlyphCacheLine> glyphCache_;
  QColor backgroundColorCache_;
  GlyphCacheLine* currentWritingLine_ = NULL;

  // Fields related to mouse operation.
  QPoint positionDelta_; // For dragging on the text to move.
  double prevHoriPosition_;
  QPoint prevPositionDelta_ = { 0, 0 };
  QPoint mouseDownPostition_ = { 0, 0 };
  int prevIndex_ = -1;
  int averageLineCount_ = 0;

  void paintByRenderer();

  // These two assume functions ownership of glyphs, but don't free them.
  // However, remember to free the glyph returned from
  // `transformGlyphStroked`.
  void transformGlyphFancy(FT_Glyph glyph);
  FT_Glyph transformGlyphStroked(FT_Glyph glyph);

  void paintCache(QPainter* painter);
  void fillCache();
  void prePaint();
  void updateStroke();
  void updateRendererText();
  void preprocessGlyph(FT_Glyph* glyphPtr);

  // Callbacks
  void beginSaveLine(FT_Vector pos,
                     double sizePoint);
  void saveSingleGlyph(FT_Glyph glyph,
                       FT_Vector penPos,
                       GlyphContext gctx);
  void saveSingleGlyphImage(QImage* image,
                            QRect rect,
                            FT_Vector penPos,
                            FT_Vector advance,
                            GlyphContext gctx);

  // Functions drawing from the cache.
  void beginDrawCacheLine(QPainter* painter,
                          GlyphCacheLine& line);
  void drawCacheGlyph(QPainter* painter,
                      const GlyphCacheEntry& entry,
                      bool colorInverted = false);

  // Mouse operations.
  GlyphCacheEntry* findGlyphByMouse(QPoint position,
                                    double* outSizePoint);
  int calculateAverageLineCount();

  void flashTimerFired();

  // Mouse constants.
  constexpr static int ClickDragThreshold = 10;
  constexpr static int HorizontalUnitLength = 100;
  constexpr static int VerticalUnitLength = 150;

  // Flash timer constants.
  constexpr static int FlashIntervalMs = 250;
  constexpr static int FlashDurationMs = 3000;
};


// end of glyphcontinuous.hpp
