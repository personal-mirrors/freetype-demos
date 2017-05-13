QMAKE_CXXFLAGS += -isystem ../../freetype2/include

# To avoid conflicts with the FreeType version compiled into or used by Qt,
# we use the static library.
#
# You should adapt this to your setup.
unix|macx {
  LIBS += ../../freetype2/objs/.libs/libfreetype.a

  CONFIG += link_pkgconfig
  PKGCONFIG += libpng harfbuzz zlib bzip2
}
win32 {
  LIBS += ../../freetyp2/objs/vc2010/freetype28.lib
  LIBS += -lpng -lharfbuzz -lz -lbz2 -lm
}

CONFIG += qt debug

# we need access to internal FreeType header files
DEFINES += FT2_BUILD_LIBRARY

SOURCES += ftinspect.cpp
HEADERS += ftinspect.h

TARGET = ftinspect

QT += widgets
