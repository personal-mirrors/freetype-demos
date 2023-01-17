// customcomboboxmodels.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.


#include "customcomboboxmodels.hpp"

#include <QApplication>
#include <QPalette>

#include <freetype/ftdriver.h>
#include <freetype/ftlcdfil.h>


/////////////////////////////////////////////////////////////////////////////
//
// HintingModeComboBoxModel
//
/////////////////////////////////////////////////////////////////////////////

HintingModeComboBoxModel::HintingModeComboBoxModel(QObject* parent)
: QAbstractListModel(parent)
{
  items_[HintingMode_TrueType_v35] = {
    HintingEngineType_TrueType,
    HintingMode_TrueType_v35,
    TT_INTERPRETER_VERSION_35,
    false, false,
    "TrueType v35"
  };
  items_[HintingMode_TrueType_v38] = {
    HintingEngineType_TrueType,
    HintingMode_TrueType_v38,
    TT_INTERPRETER_VERSION_38,
    false, false,
    "TrueType v38"
  };
  items_[HintingMode_TrueType_v40] = {
    HintingEngineType_TrueType,
    HintingMode_TrueType_v40,
    TT_INTERPRETER_VERSION_40,
    false, false,
    "TrueType v40"
  };

  items_[HintingMode_CFF_FreeType] = {
    HintingEngineType_CFF,
    HintingMode_CFF_FreeType,
    FT_HINTING_FREETYPE,
    false, false,
    "CFF (FreeType)"
  };
  items_[HintingMode_CFF_Adobe] = {
    HintingEngineType_CFF,
    HintingMode_CFF_Adobe,
    FT_HINTING_ADOBE,
    false, false,
    "CFF (Adobe)"
  };
}


int
HintingModeComboBoxModel::rowCount(const QModelIndex& parent) const
{
  return items_.size();
}


QVariant
HintingModeComboBoxModel::data(const QModelIndex& index,
                               int role) const
{
  int r = index.row();
  if (r < 0 || r >= items_.size())
    return QVariant {};
  HintingModeItem const& item = items_[r];

  switch (role)
  {
  case Qt::DisplayRole:
    return item.displayName;
  case Qt::ForegroundRole:
    if (item.enabled && item.supported)
      return QVariant {};
    else
      return QApplication::palette().color(QPalette::Disabled,
                                           QPalette::Text);
  default:
    return QVariant {};
  }
}


Qt::ItemFlags
HintingModeComboBoxModel::flags(const QModelIndex& index) const
{
  int r = index.row();
  if (r < 0 || r >= items_.size())
    return Qt::ItemFlags {}; // Not selectable, not enabled.
  HintingModeItem const& item = items_[r];

  if (item.enabled && item.supported)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  return Qt::ItemFlags {};
}


int
HintingModeComboBoxModel::indexToValueForType(int index,
                                              HintingEngineType type) const
{
  if (index < 0 || index >= items_.size())
    return -1;
  HintingModeItem const& item = items_[index];
  if (!item.supported || !item.enabled || item.type != type)
    return -1;
  return item.value;
}


int
HintingModeComboBoxModel::valueToIndexForType(int value,
                                              HintingEngineType type) const
{
  for (const auto& item : items_)
  {
    if (item.type == type && item.value == value)
      return item.key;
  }
  return -1;
}


int
HintingModeComboBoxModel::indexToTTInterpreterVersion(int index) const
{
  return indexToValueForType(index, HintingEngineType_TrueType);
}


int
HintingModeComboBoxModel::indexToCFFMode(int index) const
{
  return indexToValueForType(index, HintingEngineType_CFF);
}


int
HintingModeComboBoxModel::cffModeToIndex(int mode) const
{
  return valueToIndexForType(mode, HintingEngineType_CFF);
}


int
HintingModeComboBoxModel::ttInterpreterVersionToIndex(int version) const
{
  return valueToIndexForType(version, HintingEngineType_TrueType);
}


void
HintingModeComboBoxModel::setSupportedModes(QList<int> supportedTTIVersions,
                                            QList<int> supportedCFFModes)
{
  for (auto& item : items_)
  {
    if (item.type == HintingEngineType_TrueType)
      item.supported = supportedTTIVersions.contains(item.value);
    else if (item.type == HintingEngineType_CFF)
      item.supported = supportedCFFModes.contains(item.value);
    else
      item.supported = false;
  }
}


void
HintingModeComboBoxModel::setCurrentEngineType(HintingEngineType type,
                                               bool tricky)
{
  for (auto& item : items_)
    if (!tricky)
      item.enabled = item.supported && item.type == type;
    else
      item.enabled = item.supported && item.key == HintingMode_TrueType_v35;
}


/////////////////////////////////////////////////////////////////////////////
//
// LCDFilterComboBoxModel
//
/////////////////////////////////////////////////////////////////////////////


LCDFilterComboBoxModel::LCDFilterComboBoxModel(QObject* parent)
: QAbstractListModel(parent)
{
  items_[LCDFilter_Default] = {
    FT_LCD_FILTER_DEFAULT,
    "Default"
  };
  items_[LCDFilter_Light] = {
    FT_LCD_FILTER_LIGHT,
    "Light"
  };
  items_[LCDFilter_None] = {
    FT_LCD_FILTER_NONE,
    "None"
  };
  items_[LCDFilter_Legacy] = {
    FT_LCD_FILTER_LEGACY,
    "Legacy"
  };
}


/////////////////////////////////////////////////////////////////////////////
//
// AntiAliasingComboBoxModel
//
/////////////////////////////////////////////////////////////////////////////


AntiAliasingComboBoxModel::AntiAliasingComboBoxModel(QObject* parent)
: QAbstractListModel(parent)
{
  items_[AntiAliasing_None] = {
    {FT_LOAD_TARGET_MONO, FT_RENDER_MODE_MONO, false},
    "None"
  };
  items_[AntiAliasing_Normal] = {
    {FT_LOAD_TARGET_NORMAL, FT_RENDER_MODE_NORMAL, false},
    "Normal"
  };
  items_[AntiAliasing_Light] = {
    {FT_LOAD_TARGET_LIGHT, FT_RENDER_MODE_LIGHT, false},
    "Light"
  };
  items_[AntiAliasing_Light_SubPixel] = {
    {FT_LOAD_TARGET_LIGHT, FT_RENDER_MODE_LIGHT, false},
    "Light (Sub Pixel)"
  };
  items_[AntiAliasing_LCD] = {
    {FT_LOAD_TARGET_LCD, FT_RENDER_MODE_LCD, false},
    "LCD (RGB)"
  };
  items_[AntiAliasing_LCD_BGR] = {
    {FT_LOAD_TARGET_LCD, FT_RENDER_MODE_LCD, true},
    "LCD (BGR)"
  };
  items_[AntiAliasing_LCD_Vertical] = {
    {FT_LOAD_TARGET_LCD_V, FT_RENDER_MODE_LCD_V, false},
    "LCD (vert. RGB)"
  };
  items_[AntiAliasing_LCD_Vertical_BGR] = {
    {FT_LOAD_TARGET_LCD_V, FT_RENDER_MODE_LCD_V, true},
    "LCD (vert. BGR)"
  };

  lightAntiAliasingEnabled_ = true;
}


QVariant
AntiAliasingComboBoxModel::data(const QModelIndex& index,
                                int role) const
{
  auto row = index.row();
  if (role == Qt::ForegroundRole)
    if ((row == AntiAliasing_Light || row == AntiAliasing_Light_SubPixel)
        && !lightAntiAliasingEnabled_)
      return QApplication::palette().color(QPalette::Disabled,
                                           QPalette::Text);
  return SimpleComboBoxModelImpl::data(index, role);
}


Qt::ItemFlags
AntiAliasingComboBoxModel::flags(const QModelIndex& index) const
{
  auto row = index.row();
  if ((row == AntiAliasing_Light || row == AntiAliasing_Light_SubPixel)
      && !lightAntiAliasingEnabled_)
    return Qt::ItemFlags {};
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}


// end of customcomboboxmodels.cpp
