// fontinfo.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include <set>
#include <vector>
#include <QByteArray>
#include <QString>
#include <freetype/freetype.h>
#include <freetype/ftsnames.h>

class Engine;
struct SFNTName
{
  unsigned short nameID;
  unsigned short platformID;
  unsigned short encodingID;
  unsigned short languageID;
  QByteArray strBuf;
  QString str;
  QString langTag;
  bool strValid = false;

  static void get(Engine* engine,
                  std::vector<SFNTName>& list);
  static QString sfntNameToQString(FT_SfntName const& sfntName,
                                   bool* outSuccess = NULL);
  static QString sfntNameToQString(SFNTName const& sfntName,
                                   bool* outSuccess = NULL);
  static QString sfntNameToQString(unsigned short platformID,
                                   unsigned short encodingID, 
                                   char const* str, size_t size,
                                   bool* outSuccess = NULL);
  static QString utf16BEToQString(char const* str, size_t size);


  friend bool
  operator==(const SFNTName& lhs,
             const SFNTName& rhs)
  {
    return lhs.nameID == rhs.nameID
      && lhs.platformID == rhs.platformID
      && lhs.encodingID == rhs.encodingID
      && lhs.languageID == rhs.languageID
      && lhs.strBuf == rhs.strBuf
      && lhs.langTag == rhs.langTag;
  }


  friend bool
  operator!=(const SFNTName& lhs,
             const SFNTName& rhs)
  {
    return !(lhs == rhs);
  }
};


// end of fontinfo.hpp
