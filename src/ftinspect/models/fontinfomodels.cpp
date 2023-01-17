// fontinfomodels.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "../engine/engine.hpp"
#include "fontinfomodels.hpp"

#include <cstdint>


int
FixedSizeInfoModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;
  return static_cast<int>(storage_.size());
}


int
FixedSizeInfoModel::columnCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;
  return FSIM_Max;
}


QVariant
FixedSizeInfoModel::data(const QModelIndex& index,
                         int role) const
{
  if (index.row() < 0 || index.column() < 0)
    return {};
  auto r = static_cast<size_t>(index.row());
  if ((role != Qt::DisplayRole && role != Qt::ToolTipRole)
      || r > storage_.size())
    return {};

  auto& obj = storage_[r];
  switch (static_cast<Columns>(index.column()))
  {
  case FSIM_Height:
    return obj.height;
  case FSIM_Width:
    return obj.width;
  case FSIM_Size:
    return obj.size;
  case FSIM_XPpem:
    return obj.xPpem;
  case FSIM_YPpem:
    return obj.yPpem;
  default:
    break;
  }

  return {};
}


QVariant
FixedSizeInfoModel::headerData(int section,
                               Qt::Orientation orientation,
                               int role) const
{
  if (role != Qt::DisplayRole)
    return {};
  if (orientation == Qt::Vertical)
    return section;
  if (orientation != Qt::Horizontal)
    return {};

  switch (static_cast<Columns>(section))
  {
  case FSIM_Height:
    return tr("Height");
  case FSIM_Width:
    return tr("Width");
  case FSIM_Size:
    return tr("Size");
  case FSIM_XPpem:
    return tr("X ppem");
  case FSIM_YPpem:
    return tr("Y ppem");
  default:
    break;
  }

  return {};
}


int
CharMapInfoModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;
  return static_cast<int>(storage_.size());
}


int
CharMapInfoModel::columnCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;
  return CMIM_Max;
}


QVariant
CharMapInfoModel::data(const QModelIndex& index,
                       int role) const
{
  // TODO reduce duplication
  if (index.row() < 0 || index.column() < 0)
    return {};
  auto r = static_cast<size_t>(index.row());
  if ((role != Qt::DisplayRole && role != Qt::ToolTipRole)
      || r > storage_.size())
    return {};

  auto& obj = storage_[r];
  switch (static_cast<Columns>(index.column()))
  {
  case CMIM_Platform:
    return QString("%1 {%2}")
             .arg(obj.platformID)
             .arg(*mapTTPlatformIDToName(obj.platformID));
  case CMIM_Encoding:
    return QString("%1 {%2}")
             .arg(obj.encodingID)
             .arg(*obj.encodingName);
  case CMIM_FormatID:
    return static_cast<long long>(obj.formatID);
  case CMIM_Language:
    return static_cast<unsigned long long>(obj.languageID);
  case CMIM_MaxIndex:
    return obj.maxIndex;
  default:
    break;
  }

  return {};
}


QVariant
CharMapInfoModel::headerData(int section,
                             Qt::Orientation orientation,
                             int role) const
{
  if (role != Qt::DisplayRole)
    return {};
  if (orientation == Qt::Vertical)
    return section;
  if (orientation != Qt::Horizontal)
    return {};

  switch (static_cast<Columns>(section))
  {
  case CMIM_Platform:
    return "Platform";
  case CMIM_Encoding:
    return "Encoding";
  case CMIM_FormatID:
    return "Format ID";
  case CMIM_Language:
    return "Language";
  case CMIM_MaxIndex:
    return "Max Code Point";
  default:
    break;
  }

  return {};
}


int
SFNTNameModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;
  return static_cast<int>(storage_.size());
}


int
SFNTNameModel::columnCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;
  return SNM_Max;
}


QVariant
SFNTNameModel::data(const QModelIndex& index,
                    int role) const
{
  if (index.row() < 0 || index.column() < 0)
    return {};

  if (role == Qt::ToolTipRole && index.column() == SNM_Content)
    return tr("Double click to view the string.");

  auto r = static_cast<size_t>(index.row());
  if ((role != Qt::DisplayRole && role != Qt::ToolTipRole)
      || r > storage_.size())
    return {};

  auto& obj = storage_[r];
  switch (static_cast<Columns>(index.column()))
  {
  case SNM_Name:
    if (obj.nameID >= 256)
      return QString::number(obj.nameID);
    return QString("%1 {%2}").arg(QString::number(obj.nameID),
                                  *mapSFNTNameIDToName(obj.nameID));

  case SNM_Platform:
    return QString("%1 {%2}")
             .arg(obj.platformID)
             .arg(*mapTTPlatformIDToName(obj.platformID));
  case SNM_Encoding:
    return QString("%1 {%2}")
             .arg(obj.encodingID)
             .arg(*mapTTEncodingIDToName(obj.platformID, obj.encodingID));
  case SNM_Language:
    if (obj.languageID >= 0x8000)
      return obj.langTag + "(lang tag)";
    if (obj.platformID == 3)
      return QString("0x%1 {%2}")
               .arg(obj.languageID, 4, 16, QChar('0'))
               .arg(*mapTTLanguageIDToName(obj.platformID, obj.languageID));
    return QString("%1 {%2}")
             .arg(obj.languageID)
             .arg(*mapTTLanguageIDToName(obj.platformID, obj.languageID));
  case SNM_Content:
    return obj.str;
  default:
    break;
  }

  return {};
}


QVariant
SFNTNameModel::headerData(int section,
                          Qt::Orientation orientation,
                          int role) const
{
  if (role != Qt::DisplayRole)
    return {};
  if (orientation == Qt::Vertical)
    return section;
  if (orientation != Qt::Horizontal)
    return {};

  switch (static_cast<Columns>(section))
  {
  case SNM_Name:
    return "Name";
  case SNM_Platform:
    return "Platform";
  case SNM_Encoding:
    return "Encoding";
  case SNM_Language:
    return "Language";
  case SNM_Content:
    return "Content";
  default:
    break;
  }

  return {};
}


QString
tagToString(unsigned long tag)
{
  QString str(4, '0');
  str[0] = static_cast<char>(tag >> 24);
  str[1] = static_cast<char>(tag >> 16);
  str[2] = static_cast<char>(tag >> 8);
  str[3] = static_cast<char>(tag);
  return str;
}


int
SFNTTableInfoModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;
  return static_cast<int>(storage_.size());
}


int
SFNTTableInfoModel::columnCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;
  return STIM_Max;
}


QVariant
SFNTTableInfoModel::data(const QModelIndex& index,
                         int role) const
{
  if (index.row() < 0 || index.column() < 0)
    return {};
  auto r = static_cast<size_t>(index.row());
  if ((role != Qt::DisplayRole && role != Qt::ToolTipRole)
      || r > storage_.size())
    return {};

  auto& obj = storage_[r];
  switch (static_cast<Columns>(index.column()))
  {
  case STIM_Tag:
    return tagToString(obj.tag);
  case STIM_Offset:
    return static_cast<unsigned long long>(obj.offset);
  case STIM_Length:
    return static_cast<unsigned long long>(obj.length);
  case STIM_Valid:
    return obj.valid;
  case STIM_SharedFaces:
    if (obj.sharedFaces.empty())
      return "[]";
    {
      auto result = QString('[') + QString::number(*obj.sharedFaces.begin());
      for (auto it = std::next(obj.sharedFaces.begin());
           it != obj.sharedFaces.end();
           ++it)
      {
        auto xStr = QString::number(*it);
        result.reserve(result.length() + xStr.length() + 2);
        result += ", ";
        result += xStr;
      }
      result += ']';
      return result;
    }
  default:
    break;
  }

  return {};
}


QVariant
SFNTTableInfoModel::headerData(int section,
                               Qt::Orientation orientation,
                               int role) const
{
  if (role != Qt::DisplayRole)
    return {};
  if (orientation == Qt::Vertical)
    return section;
  if (orientation != Qt::Horizontal)
    return {};

  switch (static_cast<Columns>(section))
  {
  case STIM_Tag:
    return "Tag";
  case STIM_Offset:
    return "Offset";
  case STIM_Length:
    return "Length";
  case STIM_Valid:
    return "Valid";
  case STIM_SharedFaces:
    return "Subfont Indices";
  default:
    ;
  }

  return {};
}


int
MMGXAxisInfoModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;
  return static_cast<int>(storage_.size());
}


int
MMGXAxisInfoModel::columnCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;
  return MAIM_Max;
}


QVariant
MMGXAxisInfoModel::data(const QModelIndex& index,
                        int role) const
{
  if (index.row() < 0 || index.column() < 0)
    return {};
  auto r = static_cast<size_t>(index.row());
  if ((role != Qt::DisplayRole && role != Qt::ToolTipRole)
      || r > storage_.size())
    return {};

  auto& obj = storage_[r];
  switch (static_cast<Columns>(index.column()))
  {
  case MAIM_Tag:
    return tagToString(obj.tag);
  case MAIM_Minimum:
    return obj.minimum;
  case MAIM_Default:
    return obj.def;
  case MAIM_Maximum:
    return obj.maximum;
  case MAIM_Hidden:
    return obj.hidden;
  case MAIM_Name:
    return obj.name;
  default:
    break;
  }

  return {};
}


QVariant
MMGXAxisInfoModel::headerData(int section,
                              Qt::Orientation orientation,
                              int role) const
{
  if (role != Qt::DisplayRole)
    return {};
  if (orientation == Qt::Vertical)
    return section;
  if (orientation != Qt::Horizontal)
    return {};

  switch (static_cast<Columns>(section))
  {
  case MAIM_Tag:
    return "Tag";
  case MAIM_Minimum:
    return "Minimum";
  case MAIM_Default:
    return "Default";
  case MAIM_Maximum:
    return "Maximum";
  case MAIM_Hidden:
    return "Hidden";
  case MAIM_Name:
    return "Name";
  default:
    ;
  }

  return {};
}


int
CompositeGlyphsInfoModel::rowCount(const QModelIndex& parent) const
{
  if (!parent.isValid())
    return static_cast<int>(glyphs_.size());
  uint64_t id = parent.internalId();
  if (id >= nodes_.size())
    return 0;
  auto gid = nodes_[id].glyphIndex;
  auto iter = glyphMapper_.find(gid);
  if (iter == glyphMapper_.end())
    return 0;
  if (iter->second > glyphs_.size())
    return 0;
  return static_cast<int>(glyphs_[iter->second]. subglyphs.size());
}


int
CompositeGlyphsInfoModel::columnCount(const QModelIndex& parent) const
{
  return CGIM_Max;
}


QModelIndex
CompositeGlyphsInfoModel::index(int row,
                                int column,
                                const QModelIndex& parent) const
{
  long long parentIdx = -1; // Node index.
  if (parent.isValid()) // Not top-level.
    parentIdx = static_cast<long long>(parent.internalId());
  if (parentIdx < 0)
    parentIdx = -1;
  // Find existing node by row and parent index, -1 for top-level.
  auto lookupPair = std::pair<int, long long>(row, parentIdx);

  auto iter = nodeLookup_.find(lookupPair);
  if (iter != nodeLookup_.end())
  {
    if (iter->second < 0
        || static_cast<size_t>(iter->second) >= nodes_.size())
      return {};
    return createIndex(row, column, iter->second);
  }

  int glyphIndex = -1;
  CompositeGlyphInfo::SubGlyph const* sgInfo = nullptr;
  if (!parent.isValid()) // Top-level nodes.
    glyphIndex = glyphs_[row].index;
  else if (parent.internalId() < nodes_.size())
  {
    auto& parentInfoIndex = nodes_[parent.internalId()].glyphInfoIndex;
    if (parentInfoIndex < 0
        || static_cast<size_t>(parentInfoIndex) > glyphs_.size())
      return {};

    auto& sg = glyphs_[parentInfoIndex].subglyphs;
    glyphIndex = sg[row].index;
    sgInfo = &sg[row];
  }

  if (glyphIndex < 0)
    return {};

  ptrdiff_t glyphInfoIndex = -1;
  auto iterGlyphInfoIter = glyphMapper_.find(glyphIndex);
  if (iterGlyphInfoIter != glyphMapper_.end())
    glyphInfoIndex = static_cast<ptrdiff_t>(iterGlyphInfoIter->second);

  InfoNode node = {
    parentIdx,
    row,
    glyphIndex,
    glyphInfoIndex,
    sgInfo
  };
  nodes_.push_back(node);
  nodeLookup_.emplace(std::pair<int, long long>(row, parentIdx),
                      nodes_.size() - 1);

  return createIndex(row, column, nodes_.size() - 1);
}


QModelIndex
CompositeGlyphsInfoModel::parent(const QModelIndex& child) const
{
  if (!child.isValid())
    return {};

  auto id = static_cast<long long>(child.internalId());
  if (id < 0 || static_cast<size_t>(id) >= nodes_.size())
    return {};

  auto pid = nodes_[id].parentNodeIndex;
  if (pid < 0 || static_cast<size_t>(pid) >= nodes_.size())
    return {};

  auto& p = nodes_[pid];
  return createIndex(p.indexInParent, 0, pid);
}


QString
generatePositionTransformationText(CompositeGlyphInfo::SubGlyph const& info)
{
  QString result;
  switch (info.transformationType)
  {
  case CompositeGlyphInfo::SubGlyph::TT_UniformScale:
    result += QString("scale: %1, ")
                .arg(QString::number(info.transformation[0]));
    break;
  case CompositeGlyphInfo::SubGlyph::TT_XYScale:
    result += QString("xy scale: (%1, %2), ")
                .arg(QString::number(info.transformation[0]),
                     QString::number(info.transformation[1]));
    break;
  case CompositeGlyphInfo::SubGlyph::TT_Matrix:
    result += QString("2x2 scale: [%1, %2; %3, %4], ")
                .arg(QString::number(info.transformation[0]),
                     QString::number(info.transformation[1]),
                     QString::number(info.transformation[2]),
                     QString::number(info.transformation[3]));
    break;
  }

  switch (info.positionType)
  {
  case CompositeGlyphInfo::SubGlyph::PT_Offset:
    if (info.positionScaled)
      result += QString("scaled offset: (%1, %2)")
                  .arg(QString::number(info.position.first),
                       QString::number(info.position.second));
    else
      result += QString("offset: (%1, %2)")
                  .arg(QString::number(info.position.first),
                       QString::number(info.position.second));
    break;
  case CompositeGlyphInfo::SubGlyph::PT_Align:
      result += QString("anchor points: %1 (parent) <- %2 (this glyph)")
                  .arg(QString::number(info.position.first),
                       QString::number(info.position.second));
    break;
  }

  return result;
}


QVariant
CompositeGlyphsInfoModel::data(const QModelIndex& index,
                               int role) const
{
  if (!index.isValid())
    return {};

  auto id = index.internalId();
  if (id >= nodes_.size())
    return {};
  auto& n = nodes_[id];
  auto glyphIdx = n.glyphIndex;

  if (role == Qt::DecorationRole && index.column() == CGIM_Glyph)
  {
    auto glyphIndex = n.glyphIndex;
    auto iter = glyphIcons_.find(glyphIndex);
    if (iter == glyphIcons_.end())
      iter = glyphIcons_.emplace(glyphIndex, renderIcon(glyphIndex)).first;

    auto& pixmap = iter->second;
    if (pixmap.isNull())
      return {};
    return pixmap;
  }

  if (role != Qt::DisplayRole)
    return {};

  switch (static_cast<Columns>(index.column()))
  {
  case CGIM_Glyph:
    if (engine_->currentFontHasGlyphName())
      return QString("%1 {%2}")
               .arg(glyphIdx).arg(engine_->glyphName(glyphIdx));
    return QString::number(glyphIdx);
  case CGIM_Flag:
    if (!n.subGlyphInfo)
      return {};
    return QString("0x%1")
             .arg(n.subGlyphInfo->flag, 4, 16, QLatin1Char('0'));
  case CGIM_PositionTransformation:
    if (!n.subGlyphInfo)
      return {};
    return generatePositionTransformationText(*n.subGlyphInfo);
  default:
    ;
  }

  return {};
}


QVariant
CompositeGlyphsInfoModel::headerData(int section,
                                     Qt::Orientation orientation,
                                     int role) const
{
  if (role != Qt::DisplayRole)
    return {};
  if (orientation != Qt::Horizontal)
    return {};

  switch (static_cast<Columns>(section))
  {
  case CGIM_Glyph:
    return tr("Glyph");
  case CGIM_Flag:
    return tr("Flags");
  case CGIM_PositionTransformation:
    return tr("Position and Transformation");
  default:
    ;
  }

  return {};
}


int
CompositeGlyphsInfoModel::glyphIndexFromIndex(const QModelIndex& idx)
{
  if (!idx.isValid())
    return -1;

  auto id = idx.internalId();
  if (id >= nodes_.size())
    return -1;
  auto& n = nodes_[id];
  return n.glyphIndex;
}


void
CompositeGlyphsInfoModel::beginModelUpdate()
{
  beginResetModel();
  glyphs_.clear();
  nodeLookup_.clear();
  nodes_.clear();
}


void
CompositeGlyphsInfoModel::endModelUpdate()
{
  glyphMapper_.clear();
  for (size_t i = 0; i < glyphs_.size(); i++)
    glyphMapper_.emplace(glyphs_[i].index, i);

  glyphIcons_.clear();
  endResetModel();
}


QPixmap
CompositeGlyphsInfoModel::renderIcon(int glyphIndex) const
{
  engine_->setSizeByPixel(20); // This size is arbitrary
  if (!engine_->currentPalette())
    engine_->loadPalette();
  auto image = engine_->renderingEngine()
                      ->tryDirectRenderColorLayers(glyphIndex, NULL, false);
  if (!image)
  {
    auto glyph = engine_->loadGlyph(glyphIndex);
    if (!glyph)
      return {};
    image = engine_->renderingEngine()
                   ->convertGlyphToQImage(glyph, NULL, false);
  }

  if (!image)
    return {};

  auto result = engine_->renderingEngine()->padToSize(image, 20);
  delete image;
  return result;
}


// end of fontinfomodels.cpp
