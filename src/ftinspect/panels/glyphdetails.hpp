// glyphdetails.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once

#include "../glyphcomponents/glyphbitmap.hpp"

#include <QBoxLayout>
#include <QButtonGroup>
#include <QGridLayout>
#include <QImage>
#include <QLabel>
#include <QRadioButton>
#include <QWidget>

#include <freetype/freetype.h>


struct GlyphCacheEntry;
class Engine;

class GlyphDetails
: public QWidget
{
  Q_OBJECT

public:
  GlyphDetails(QWidget* parent,
               Engine* engine);
  ~GlyphDetails() override;

  void updateGlyph(GlyphCacheEntry& ctxt,
                   int charMapIndex);

signals:
  void switchToSingular(int index);
  void closeDockWidget();

protected:
  void keyReleaseEvent(QKeyEvent* event) override;

private:
  Engine* engine_ = NULL;
  int glyphIndex_ = -1;

  enum DisplayUnit : int
  {
    DU_FontUnit,
    DU_Point,
    DU_Pixel
  };

  QButtonGroup* unitButtonGroup_;
  QRadioButton* fontUnitButton_;
  QRadioButton* pointButton_;
  QRadioButton* pixelButton_;

  QLabel* glyphIndexPromptLabel_;
  QLabel* charCodePromptLabel_;
  QLabel* glyphNamePromptLabel_;
  QLabel* bboxSizePromptLabel_;
  QLabel* horiBearingPromptLabel_;
  QLabel* horiAdvancePromptLabel_;
  QLabel* vertBearingPromptLabel_;
  QLabel* vertAdvancePromptLabel_;
  QLabel* inkSizePromptLabel_;
  QLabel* bitmapOffsetPromptLabel_;

  QLabel* glyphIndexLabel_;
  QLabel* charCodeLabel_;
  QLabel* glyphNameLabel_;
  QLabel* bboxSizeLabel_;
  QLabel* horiBearingLabel_;
  QLabel* horiAdvanceLabel_;
  QLabel* vertBearingLabel_;
  QLabel* vertAdvanceLabel_;
  QLabel* inkSizeLabel_;
  QLabel* bitmapOffsetLabel_;

  GlyphBitmapWidget* bitmapWidget_;

  QHBoxLayout* unitLayout_;
  QGridLayout* layout_;

  int dpi_;
  FT_Glyph_Metrics fontUnitMetrics_;
  FT_Glyph_Metrics pixelMetrics_;

  void createLayout();
  void createConnections();

  void changeUnit(int unitId);
  void bitmapWidgetClicked();
};


// end of glyphdetails.hpp
