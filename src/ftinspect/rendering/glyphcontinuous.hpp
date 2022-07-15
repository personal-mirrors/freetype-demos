// glyphcontinuous.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include "graphicsdefault.hpp"

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
    M_Stroked,
    M_Waterfall
  };

  int displayingCount() { return displayingCount_; }

  // all those setters don't trigger repaint.
  void setBeginIndex(int index) { beginIndex_ = index; }
  void setLimitIndex(int index) { limitIndex_ = index; }
  void setCharMapIndex(int index) { charMapIndex_ = index; }
  void setSource(Source mode) { source_ = mode; }
  void setMode(Mode mode) { mode_ = mode; }
  void setFancyParams(double boldX, double boldY, double slant)
  {
    boldX_ = boldX;
    boldY_ = boldY;
    slant_ = slant;
  }
  void setStrokeRadius(double radius) { strokeRadius_ = radius; }
  void setRotation(double rotation) { rotation_ = rotation; }
  void setVertical(bool vertical) { vertical_ = vertical; }
  void setSourceText(QString text) { text_ = std::move(text); }


signals:
  void wheelNavigate(int steps);
  void wheelResize(int steps);
  void displayingCountUpdated(int newCount);

protected:
  void paintEvent(QPaintEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

private:
  Engine* engine_;
  GraphicsDefault* graphicsDefault_;

  Source source_ = SRC_AllGlyphs;
  Mode mode_ = M_Normal;
  int beginIndex_;
  int limitIndex_;
  int charMapIndex_;
  double boldX_, boldY_, slant_;
  double strokeRadius_;
  double rotation_;
  bool vertical_;
  QString text_;

  int displayingCount_ = 0;
  FT_Size_Metrics metrics_;
  int x_ = 0, y_ = 0;
  int stepY_ = 0;
  FT_Glyph glyph_;
  FT_Outline outline_;
  // when glyph is cloned, outline is factually also cloned
  // but `isOutlineCloned` won't be set!
  bool isGlyphCloned_ = false, isOutlineCloned_ = false;

  FT_Stroker stroker_;

  void paintAG(QPainter* painter);
  void transformGlyphAGFancy();
  void transformGlyphAGStroked();
  void prePaint();
  // return if there's enough space to paint the current char
  bool paintChar(QPainter* painter);
  bool loadGlyph(int index);
  void cloneGlyph();
  void cloneOutline();
  void cleanCloned();

  bool checkFitX(int x);
  bool checkFitY(int y);
};


// end of glyphcontinuous.hpp
