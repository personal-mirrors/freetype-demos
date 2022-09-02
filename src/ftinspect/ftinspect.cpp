// ftinspect.cpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#include "maingui.hpp"
#include "engine/engine.hpp"

#include <QApplication>

#define VERSION "X.Y.Z"


int
main(int argc,
     char** argv)
{
  QApplication app(argc, argv);
  app.setApplicationName("ftinspect");
  app.setApplicationVersion(VERSION);
  app.setOrganizationName("FreeType");
  app.setOrganizationDomain("freetype.org");

  Engine engine;
  MainGUI gui(&engine);
  gui.setDefaults();

  gui.show();

  return app.exec();
}


// end of ftinspect.cpp
