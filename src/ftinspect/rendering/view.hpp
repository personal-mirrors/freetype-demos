#pragma once

#include <QGraphicsItem>
#include <QPen>

#include <ft2build.h>
#include "../engine/engine.hpp"
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_GLYPH_H
#include FT_TYPES_H
#include FT_RENDER_H
#include FT_STROKER_H

#include FT_INTERNAL_DEBUG_H

  /* showing driver name */
#include FT_MODULE_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DRIVER_H

#include FT_SYNTHESIS_H
#include FT_LCD_FILTER_H
#include FT_DRIVER_H

#include FT_COLOR_H
#include FT_BITMAP_H


class RenderAll
: public QGraphicsItem
{
public:
  RenderAll(FT_Face face,
       FT_Size  size,
       FTC_Manager cacheManager,
       FTC_FaceID  face_id,
       FTC_CMapCache  cmap_cache,
       FT_Library library,
       int mode,
       FTC_ScalerRec scaler,
       FTC_ImageCache imageCache,
       double x_factor,
       double y_factor,
       double slant_factor,
       double stroke_factor);
  ~RenderAll();
  QRectF boundingRect() const;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget);

private:
  FT_Face face;
  FT_Library library;
  QRectF m_glyphRect;
  FT_Error error;
  FTC_Manager cacheManager;
  FTC_FaceID face_id;
  FTC_CMapCache cmap_cache;
  FT_Size size;
  int mode;
  Engine* engine;
  FTC_ScalerRec scaler;
  FTC_ImageCache imageCache;
  double x_factor;
  double y_factor;
  double slant_factor;
  double stroke_factor;
};


// end of glyphbitmap.hpp
