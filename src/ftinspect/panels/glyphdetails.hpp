// glyphdetails.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include "../rendering/glyphbitmap.hpp"

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QImage>
#include <freetype/freetype.h>

struct GlyphCacheEntry;
class Engine;
class GlyphDetails
: public QWidget
{
public:
  GlyphDetails(QWidget* parent, Engine* engine);
  ~GlyphDetails() override;

  void updateGlyph(GlyphCacheEntry& ctxt,
                   int charMapIndex);

private:
  Engine* engine_ = NULL;

  QLabel* glyphIndexPromptLabel_;
  QLabel* charCodePromptLabel_;
  QLabel* glyphNamePromptLabel_;

  QLabel* glyphIndexLabel_;
  QLabel* charCodeLabel_;
  QLabel* glyphNameLabel_;

  GlyphBitmapWidget* bitmapWidget_;

  QGridLayout* layout_;

  void createLayout();
  void createConnections();
};


// end of glyphdetails.hpp
