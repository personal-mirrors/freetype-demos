// glyphbitmap.hpp

// Copyright (C) 2016-2019 by Werner Lemberg.


#pragma once

#include <QGraphicsItem>
#include <QPen>

#include "../engine/engine.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H


#include FT_DRIVER_H
#include FT_LCD_FILTER_H

// internal FreeType header files; only available in the source code bundle
#include FT_INTERNAL_DRIVER_H
#include FT_INTERNAL_OBJECTS_H


class Comparator
: public QGraphicsItem
{
public:
  Comparator(FT_Library library,
             FT_Face face,
             FT_Size  size,
             QStringList fontList,
             int load_flags[],
             int pixelMode[],
             QVector<QRgb> grayColorTable,
             QVector<QRgb> monoColorTable,
             bool warping[],
             bool kerningCol[]);
  ~Comparator();
  QRectF boundingRect() const;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget);

private:
  FT_Library library;
  FT_Face f;
  FT_Face face;
  FT_Size  size;
  FT_Error error;
  Engine* engine;
  QStringList fontList;
  int load[3];
  int pixelMode_col[3];
  bool warping_col[3];
  QVector<QRgb> grayColorTable;
  QVector<QRgb> monoColorTable;
  FT_UInt kerning_mode = FT_KERNING_DEFAULT;
  bool kerning[3];
};


// end of glyphbitmap.hpp
