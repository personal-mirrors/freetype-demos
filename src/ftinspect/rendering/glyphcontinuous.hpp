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
    M_Stroked
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
  void setWaterfall(bool waterfall) { waterfall_ = waterfall; }
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

  Source source_ = SRC_AllGlyphs;
  Mode mode_ = M_Normal;
  int beginIndex_;
  int limitIndex_;
  int charMapIndex_;
  double boldX_, boldY_, slant_;
  double strokeRadius_;
  double rotation_;
  bool vertical_;
  bool waterfall_;
  QString text_;

  int displayingCount_ = 0;
  FT_Size_Metrics metrics_;
  int x_ = 0, y_ = 0;
  int stepY_ = 0;

  // Pay especially attention to life cycles & ownerships of those objects:
  // Note that outline and bitmap can be either invalid, owned by glyph or
  // owned by `this`.
  // If owned by `this`, then it's safe to do manipulation, and need to cleanup
  // If owned by glyph, then must clone to do manipulation, and no cleanup
  // In `loadGraph`, these 3 values will all be updated.
  // Note that `glyph_` is a pointer, while `outline_` and `bitmap_` are structs
  FT_Glyph glyph_;
  FT_Outline outline_; // Using outline_->points == NULL to determine validity
  FT_Bitmap bitmap_; // Using bitmap_->buffer == NULL to determine validity
  // when glyph is cloned, outline is factually also cloned
  // never manually clone your outline or you can't easily render it!
  bool isGlyphCloned_ = false;
  bool isBitmapCloned_ = false;

  FT_Stroker stroker_;

  void paintAG(QPainter* painter);
  void transformGlyphFancy();
  void transformGlyphStroked();
  void prePaint();
  // return if there's enough space to paint the current char
  bool paintChar(QPainter* painter);
  bool loadGlyph(int index);

  void cloneGlyph();
  void cloneBitmap();
  void refreshOutlineOrBitmapFromGlyph();
  void cleanCloned();

  bool checkFitX(int x);
  bool checkFitY(int y);
};


// end of glyphcontinuous.hpp
