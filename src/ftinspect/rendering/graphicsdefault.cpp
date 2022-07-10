// graphicsdefault.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "graphicsdefault.hpp"

GraphicsDefault* GraphicsDefault::instance_ = NULL;

GraphicsDefault::GraphicsDefault()
{
  // color tables (with suitable opacity values) for converting
  // FreeType's pixmaps to something Qt understands
  monoColorTable.append(QColor(Qt::transparent).rgba());
  monoColorTable.append(QColor(Qt::black).rgba());

  for (int i = 0xFF; i >= 0; i--)
    grayColorTable.append(qRgba(i, i, i, 0xFF - i));

  // XXX make this user-configurable

  axisPen.setColor(Qt::black);
  axisPen.setWidth(0);
  blueZonePen.setColor(QColor(64, 64, 255, 64)); // light blue
  blueZonePen.setWidth(0);
  gridPen.setColor(Qt::lightGray);
  gridPen.setWidth(0);
  offPen.setColor(Qt::darkGreen);
  offPen.setWidth(3);
  onPen.setColor(Qt::red);
  onPen.setWidth(3);
  outlinePen.setColor(Qt::red);
  outlinePen.setWidth(0);
  segmentPen.setColor(QColor(64, 255, 128, 64)); // light green
  segmentPen.setWidth(0);
}


GraphicsDefault*
GraphicsDefault::deafultInstance()
{
  if (!instance_)
    instance_ = new GraphicsDefault;

  return instance_;
}


// end of graphicsdefault.cpp
