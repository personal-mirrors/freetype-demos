// fontinfo.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include <set>
#include <QDateTime>
#include <QByteArray>
#include <QString>
#include <freetype/freetype.h>
#include <freetype/ftsnames.h>
#include <freetype/t1tables.h>

class Engine;

struct SFNTTableInfo
{
  unsigned long tag = 0;
  unsigned long offset = 0;
  unsigned long length = 0;
  bool valid = false;
  std::set<unsigned long> sharedFaces;

  static void getForAll(Engine* engine, std::vector<SFNTTableInfo>& infos);


  friend bool
  operator==(const SFNTTableInfo& lhs,
             const SFNTTableInfo& rhs)
  {
    return lhs.tag == rhs.tag
      && lhs.offset == rhs.offset
      && lhs.length == rhs.length
      && lhs.valid == rhs.valid
      && lhs.sharedFaces == rhs.sharedFaces;
  }


  friend bool
  operator!=(const SFNTTableInfo& lhs,
             const SFNTTableInfo& rhs)
  {
    return !(lhs == rhs);
  }
};


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


struct FontBasicInfo
{
  int numFaces = -1;
  QString familyName;
  QString styleName;
  QString postscriptName;
  QDateTime createdTime;
  QDateTime modifiedTime;
  QString revision;
  QString copyright;
  QString trademark;
  QString manufacturer;

  static FontBasicInfo get(Engine* engine);


  // Oh, we have no C++20 :(
  friend bool
  operator==(const FontBasicInfo& lhs,
             const FontBasicInfo& rhs)
  {
    return lhs.numFaces == rhs.numFaces
      && lhs.familyName == rhs.familyName
      && lhs.styleName == rhs.styleName
      && lhs.postscriptName == rhs.postscriptName
      && lhs.createdTime == rhs.createdTime
      && lhs.modifiedTime == rhs.modifiedTime
      && lhs.revision == rhs.revision
      && lhs.copyright == rhs.copyright
      && lhs.trademark == rhs.trademark
      && lhs.manufacturer == rhs.manufacturer;
  }


  friend bool
  operator!=(const FontBasicInfo& lhs,
             const FontBasicInfo& rhs)
  {
    return !(lhs == rhs);
  }
};


struct FontTypeEntries
{
  QString driverName;
  bool sfnt          : 1;
  bool scalable      : 1;
  bool mmgx          : 1;
  bool fixedSizes    : 1;
  bool hasHorizontal : 1;
  bool hasVertical   : 1;
  bool fixedWidth    : 1;
  bool glyphNames    : 1;

  int emSize;
  FT_BBox globalBBox;
  int ascender;
  int descender;
  int height;
  int maxAdvanceWidth;
  int maxAdvanceHeight;
  int underlinePos;
  int underlineThickness;

  static FontTypeEntries get(Engine* engine);


  // Oh, we have no C++20 :(
  friend bool
  operator==(const FontTypeEntries& lhs,
             const FontTypeEntries& rhs)
  {
    return lhs.driverName == rhs.driverName
      && lhs.sfnt == rhs.sfnt
      && lhs.scalable == rhs.scalable
      && lhs.mmgx == rhs.mmgx
      && lhs.fixedSizes == rhs.fixedSizes
      && lhs.hasHorizontal == rhs.hasHorizontal
      && lhs.hasVertical == rhs.hasVertical
      && lhs.fixedWidth == rhs.fixedWidth
      && lhs.glyphNames == rhs.glyphNames
      && lhs.emSize == rhs.emSize
      && lhs.globalBBox.xMax == rhs.globalBBox.xMax
      && lhs.globalBBox.xMin == rhs.globalBBox.xMin
      && lhs.globalBBox.yMax == rhs.globalBBox.yMax
      && lhs.globalBBox.yMin == rhs.globalBBox.yMin
      && lhs.ascender == rhs.ascender
      && lhs.descender == rhs.descender
      && lhs.height == rhs.height
      && lhs.maxAdvanceWidth == rhs.maxAdvanceWidth
      && lhs.maxAdvanceHeight == rhs.maxAdvanceHeight
      && lhs.underlinePos == rhs.underlinePos
      && lhs.underlineThickness == rhs.underlineThickness;
  }


  friend bool
  operator!=(const FontTypeEntries& lhs,
             const FontTypeEntries& rhs)
  {
    return !(lhs == rhs);
  }
};


// For PostScript `PS_FontInfoRec` and `PS_PrivateRec`, we don't create our own
// structs but direct use the ones provided by FreeType.
// But we still need to provided `operator==`
// No operator== for PS_FontInfoRec since there's little point to deep-copy it
// bool operator==(const PS_FontInfoRec& lhs, const PS_FontInfoRec& rhs);
bool operator==(const PS_PrivateRec& lhs, const PS_PrivateRec& rhs);


struct FontFixedSize
{
  short height;
  short width;
  double size;
  double xPpem;
  double yPpem;


  // Returns that if the list is updated
  // Using a callback because Qt needs `beginResetModel` to be called **before**
  // the internal storage updates.
  static bool get(Engine* engine,
                  std::vector<FontFixedSize>& list,
                  const std::function<void()>& onUpdateNeeded);


  friend bool
  operator==(const FontFixedSize& lhs,
             const FontFixedSize& rhs)
  {
    return lhs.height == rhs.height
      && lhs.width == rhs.width
      && lhs.size == rhs.size
      && lhs.xPpem == rhs.xPpem
      && lhs.yPpem == rhs.yPpem;
  }


  friend bool
  operator!=(const FontFixedSize& lhs,
             const FontFixedSize& rhs)
  {
    return !(lhs == rhs);
  }
};


struct CompositeGlyphInfo
{
  struct SubGlyph
  {
    enum PositionType
    {
      PT_Offset, // Child's points are added with a xy-offset
      PT_Align // One point of the child is aligned with one point of the parent
    };
    unsigned short index;
    unsigned short flag;
    PositionType positionType;
    // For PT_Offset: <deltaX, deltaY>
    // For PT_Align:  <childPoint, parentPoint>
    std::pair<short, short> position;

    SubGlyph(unsigned short index,
             unsigned short flag,
             PositionType positionType,
             std::pair<short, short> position)
    : index(index),
      flag(flag),
      positionType(positionType),
      position(std::move(position))
    { }


    friend bool
    operator==(const SubGlyph& lhs,
               const SubGlyph& rhs)
    {
      return lhs.index == rhs.index
        && lhs.flag == rhs.flag
        && lhs.positionType == rhs.positionType
        && lhs.position == rhs.position;
    }


    friend bool
    operator!=(const SubGlyph& lhs,
               const SubGlyph& rhs)
    {
      return !(lhs == rhs);
    }
  };


  int index;
  std::vector<SubGlyph> subglyphs;


  CompositeGlyphInfo(short index,
                     std::vector<SubGlyph> subglyphs)
  : index(index),
    subglyphs(std::move(subglyphs))
  { }


  friend bool
  operator==(const CompositeGlyphInfo& lhs,
             const CompositeGlyphInfo& rhs)
  {
    return lhs.index == rhs.index
      && lhs.subglyphs == rhs.subglyphs;
  }


  friend bool
  operator!=(const CompositeGlyphInfo& lhs,
             const CompositeGlyphInfo& rhs)
  {
    return !(lhs == rhs);
  }


  // expensive
  static void get(Engine* engine, std::vector<CompositeGlyphInfo>& list);
};


QString* mapSFNTNameIDToName(unsigned short nameID);
QString* mapTTPlatformIDToName(unsigned short platformID);
QString* mapTTEncodingIDToName(unsigned short platformID,
                               unsigned short encodingID);
QString* mapTTLanguageIDToName(unsigned short platformID,
                               unsigned short languageID);


// end of fontinfo.hpp
