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
  case FSIM_Index:
    return index.row();
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
  if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
    return {};

  switch (static_cast<Columns>(section))
  {
  case FSIM_Index:
    return tr("#");
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
  case CMIM_Index: 
    return index.row();
  case CMIM_Encoding:
    return *obj.encodingName;
  case CMIM_PlatformID: 
    return obj.platformID;
  case CMIM_EncodingID: 
    return obj.encodingID;
  case CMIM_FormatID: 
    return static_cast<long long>(obj.formatID);
  case CMIM_LanguageID:
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
  if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
    return {};

  switch (static_cast<Columns>(section))
  {
  case CMIM_Index:
    return "#";
  case CMIM_Encoding:
    return "Encoding";
  case CMIM_PlatformID:
    return "Platform ID";
  case CMIM_EncodingID:
    return "Encoding ID";
  case CMIM_FormatID:
    return "Format ID";
  case CMIM_LanguageID:
    return "Language ID";
  case CMIM_MaxIndex:
    return "Max Code Point";
  default:
    break;
  }

  return {};
}


// end of fontinfomodels.cpp
