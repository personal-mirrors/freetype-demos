#include "comparator.hpp"

#include <cmath>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QFile>
#include <QImage>
#include <iostream>
#include <QtDebug>


static const char*  default_text =
  "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Cras sit amet"
  " dui.  Nam sapien. Fusce vestibulum ornare metus. Maecenas ligula orci,"
  " consequat vitae, dictum nec, lacinia non, elit. Aliquam iaculis"
  " molestie neque. Maecenas suscipit felis ut pede convallis malesuada."
  " Aliquam erat volutpat. Nunc pulvinar condimentum nunc. Donec ac sem vel"
  " leo bibendum aliquam. Pellentesque habitant morbi tristique senectus et"
  " netus et malesuada fames ac turpis egestas.\n"
  "\n"
  "Sed commodo. Nulla ut libero sit amet justo varius blandit. Mauris vitae"
  " nulla eget lorem pretium ornare. Proin vulputate erat porta risus."
  " Vestibulum malesuada, odio at vehicula lobortis, nisi metus hendrerit"
  " est, vitae feugiat quam massa a ligula. Aenean in tellus. Praesent"
  " convallis. Nullam vel lacus.  Aliquam congue erat non urna mollis"
  " faucibus. Morbi vitae mauris faucibus quam condimentum ornare. Quisque"
  " sit amet augue. Morbi ullamcorper mattis enim. Aliquam erat volutpat."
  " Morbi nec felis non enim pulvinar lobortis.  Ut libero. Nullam id orci"
  " quis nisl dapibus rutrum. Suspendisse consequat vulputate leo. Aenean"
  " non orci non tellus iaculis vestibulum. Sed neque.\n"
  "\n";


RenderAll::RenderAll(FT_Face face,
          FT_Size  size,
          FTC_Manager cacheManager,
          FTC_FaceID  face_id,
          FTC_CMapCache  cmap_cache,
          FT_Library lib,
          int render_mode,
          FTC_ScalerRec scaler,
          FTC_ImageCache imageCache,
          double x,
          double y,
          double slant_factor,
          double stroke_factor)
:face(face),
size(size),
cacheManager(cacheManager),
face_id(face_id),
cmap_cache(cmap_cache),
library(lib),
mode(render_mode),
scaler(scaler),
imageCache(imageCache),
x_factor(x),
y_factor(y),
slant_factor(slant_factor),
stroke_factor(stroke_factor)
{
}


RenderAll::~RenderAll()
{
  //FT_Stroker_Done(stroker);
  //FTC_Manager_Done(cacheManager);
}

QRectF
RenderAll::boundingRect() const
{
  return QRectF(-320, -200,
                640, 400);
}


void
RenderAll::paint(QPainter* painter,
                   const QStyleOptionGraphicsItem* option,
                   QWidget*)
{
}

// end of RenderAll.cpp
