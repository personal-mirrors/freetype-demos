// glyphcontinuous.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include "graphicsdefault.hpp"
#include "../engine/stringrenderer.hpp"

#include <utility>

#include <QWidget>

#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/ftstroke.h>


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

signals:
  void wheelNavigate(int steps);
  void wheelResize(int steps);
  void displayingCountUpdated(int newCount);

protected:
  void paintEvent(QPaintEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

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

  void paintByRenderer(QPainter* painter);

  // These two are used indendpent of current glyph variables
  // and assumes ownership of glyphs, but don't free them.
  // However, remember to free the glyph returned from `transformGlyphStroked`
  void transformGlyphFancy(FT_Glyph glyph);
  FT_Glyph transformGlyphStroked(FT_Glyph glyph);

  void prePaint();
  void updateRendererText();
  void preprocessGlyph(FT_Glyph* glyphPtr);
  void beginLine(QPainter* painter,
                 FT_Vector pos, 
                 double sizePoint);
  void drawSingleGlyph(QPainter* painter,
                       FT_Glyph glyph);
};


// end of glyphcontinuous.hpp
