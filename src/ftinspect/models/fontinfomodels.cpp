// fontinfomodels.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "fontinfomodels.hpp"

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
  if (role != Qt::DisplayRole || r > storage_.size())
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
  if (role != Qt::DisplayRole || r > storage_.size())
    return {};

  auto& obj = storage_[r];
  switch (static_cast<Columns>(index.column()))
  {
  case CMIM_Platform: 
    return QString("%1 <%2>")
             .arg(obj.platformID)
             .arg(*mapTTPlatformIDToName(obj.platformID));
  case CMIM_Encoding:
    return QString("%1 <%2>")
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
  if (role != Qt::DisplayRole || r > storage_.size())
    return {};

  auto& obj = storage_[r];
  switch (static_cast<Columns>(index.column()))
  {
  case SNM_Name:
    if (obj.nameID >= 256)
      return QString::number(obj.nameID);
    return QString("%1 <%2>")
             .arg(obj.nameID)
             .arg(*mapSFNTNameIDToName(obj.nameID));
  case SNM_Platform:
    return QString("%1 <%2>")
             .arg(obj.platformID)
             .arg(*mapTTPlatformIDToName(obj.platformID));
  case SNM_Encoding:
    return QString("%1 <%2>")
             .arg(obj.encodingID)
             .arg(*mapTTEncodingIDToName(obj.platformID, obj.encodingID));
  case SNM_Language:
    if (obj.languageID >= 0x8000)
      return obj.langTag + "(lang tag)";
    if (obj.platformID == 3)
      return QString("0x%1 <%2>")
               .arg(obj.languageID, 4, 16, QChar('0'))
               .arg(*mapTTLanguageIDToName(obj.platformID, obj.languageID));
    return QString("%1 <%2>")
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
  if (role != Qt::DisplayRole || r > storage_.size())
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
  default:;
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
  if (role != Qt::DisplayRole || r > storage_.size())
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
  default: ;
  }

  return {};
}


// end of fontinfomodels.cpp
