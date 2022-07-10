// glyphcontinuous.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include "graphicsdefault.hpp"
#include <QWidget>
#include <freetype/freetype.h>

class Engine;
class GlyphContinuous
: public QWidget
{
  Q_OBJECT
public:
  GlyphContinuous(QWidget* parent, Engine* engine);
  ~GlyphContinuous() override = default;

  enum Mode : int
  {
    AllGlyphs,
    TextString
  };

  enum SubModeAllGlyphs : int
  {
    AG_AllGlyphs,
    AG_Fancy,
    AG_Stroked,
    AG_Waterfall
  };

  int displayingCount() { return displayingCount_; }

  // all those setters don't trigger repaint.
  void setBeginIndex(int index) { beginIndex_ = index; }
  void setLimitIndex(int index) { limitIndex_ = index; }
  void setCharMapIndex(int index) { charMapIndex_ = index; }
  void setMode(Mode mode) { mode_ = mode; }
  void setSubModeAllGlyphs(SubModeAllGlyphs modeAg) { modeAG_ = modeAg; }

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

  int beginIndex_;
  int limitIndex_;
  int charMapIndex_;
  Mode mode_ = AllGlyphs;
  SubModeAllGlyphs modeAG_ = AG_AllGlyphs;

  int displayingCount_ = 0;
  FT_Size_Metrics metrics_;
  int x_ = 0, y_ = 0;
  int stepY_ = 0;

  void paintAGAllGlyphs(QPainter* painter);
  void prePaint();
  // return if there's enough space to paint the current char
  bool paintChar(QPainter* painter, int index);

  bool checkFitX(int x);
  bool checkFitY(int y);
};


// end of glyphcontinuous.hpp
