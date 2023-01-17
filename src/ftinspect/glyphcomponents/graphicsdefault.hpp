// graphicsdefault.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once

#include <QPen>
#include <QRgb>
#include <QVector>


// This is the default graphics object fed into render functions.
struct GraphicsDefault
{
  QPen axisPen;
  QPen blueZonePen;
  QPen gridPen;
  QPen offPen;
  QPen onPen;
  QPen outlinePen;
  QPen segmentPen;

  QPen advanceAuxPen;
  QPen ascDescAuxPen;

  GraphicsDefault();

  static GraphicsDefault* deafultInstance();

private:
  static GraphicsDefault* instance_;
};


// end of graphicsdefault.hpp
