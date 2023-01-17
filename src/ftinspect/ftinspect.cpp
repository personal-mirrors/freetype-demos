// ftinspect.cpp

// Copyright (C) 2016-2023 by
// Werner Lemberg.


#include "maingui.hpp"
#include "engine/engine.hpp"

#include <QApplication>

#include <freetype/freetype.h>


int
main(int argc,
     char** argv)
{
  auto version = QString("%1.%2.%3")
                   .arg(QString::number(FREETYPE_MAJOR),
                        QString::number(FREETYPE_MINOR),
                        QString::number(FREETYPE_PATCH));

  QApplication app(argc, argv);
  app.setApplicationName("ftinspect");
  app.setApplicationVersion(version);
  app.setOrganizationName("FreeType");
  app.setOrganizationDomain("freetype.org");

  Engine engine;
  MainGUI gui(&engine);

  gui.show();

  return app.exec();
}


// end of ftinspect.cpp
