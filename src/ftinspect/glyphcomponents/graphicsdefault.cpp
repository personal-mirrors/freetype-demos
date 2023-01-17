// graphicsdefault.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "graphicsdefault.hpp"


GraphicsDefault* GraphicsDefault::instance_ = NULL;

GraphicsDefault::GraphicsDefault()
{
  // XXX make this user-configurable

  axisPen.setColor(Qt::black);
  axisPen.setWidth(0);
  blueZonePen.setColor(QColor(64, 64, 255, 64)); // light blue
  blueZonePen.setWidth(0);
  // Don't make this solid.
  gridPen.setColor(QColor(0, 0, 0, 255 - QColor(Qt::lightGray).red()));
  gridPen.setWidth(0);
  offPen.setColor(Qt::darkGreen);
  offPen.setWidth(3);
  onPen.setColor(Qt::red);
  onPen.setWidth(3);
  outlinePen.setColor(Qt::red);
  outlinePen.setWidth(0);
  segmentPen.setColor(QColor(64, 255, 128, 64)); // light green
  segmentPen.setWidth(0);

  advanceAuxPen.setColor(QColor(110, 52, 235)); // kind of blue
  advanceAuxPen.setWidth(0);
  ascDescAuxPen.setColor(QColor(255, 0, 0)); // red
  ascDescAuxPen.setWidth(0);
}


GraphicsDefault*
GraphicsDefault::deafultInstance()
{
  if (!instance_)
    instance_ = new GraphicsDefault;

  return instance_;
}


// end of graphicsdefault.cpp
