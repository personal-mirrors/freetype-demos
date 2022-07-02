// ttsettingscomboboxmodel.cpp

// Copyright (C) 2022 by Charlie Jiang.


#include "ttsettingscomboboxmodel.hpp"

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
    return Qt::ItemFlags {}; // not selectable, not enabled
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
  for (auto it = items_.begin();
       it != items_.end();
       ++it)
  {
    if (it->type == type && it->value == value)
      return it->key;
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
  for (auto it = items_.begin();
       it != items_.end();
       ++it)
  {
    if (it->type == HintingEngineType_TrueType)
      it->supported = supportedTTIVersions.contains(it->value);
    else if (it->type == HintingEngineType_CFF)
      it->supported = supportedCFFModes.contains(it->value);
    else
      it->supported = false;
  }
}


void
HintingModeComboBoxModel::setCurrentEngineType(HintingEngineType type)
{
  for (auto it = items_.begin(); 
       it != items_.end(); 
       ++it)
    it->enabled = it->supported && it->type == type;
}


/////////////////////////////////////////////////////////////////////////////
//
// SimpleComboBoxModel
//
/////////////////////////////////////////////////////////////////////////////


SimpleComboBoxModel::SimpleComboBoxModel(QObject* parent)
: QAbstractListModel(parent)
{
}


int
SimpleComboBoxModel::rowCount(const QModelIndex& parent) const
{
  return items_.size();
}


QVariant
SimpleComboBoxModel::data(const QModelIndex& index, int role) const
{
  if (role != Qt::DisplayRole)
    return QVariant {};

  int r = index.row();
  if (r < 0 || r >= items_.size())
    return QVariant {};
  return items_[r].displayName;
}


int
SimpleComboBoxModel::indexToValue(int index)
{
  if (index < 0 || index >= items_.size())
    return -1;
  return items_[index].value;
}


/////////////////////////////////////////////////////////////////////////////
//
// LCDFilterComboBoxModel
//
/////////////////////////////////////////////////////////////////////////////


LCDFilterComboBoxModel::LCDFilterComboBoxModel(QObject* parent)
: SimpleComboBoxModel(parent)
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
: SimpleComboBoxModel(parent)
{
  items_[AntiAliasing_None] = {
    FT_LOAD_TARGET_MONO,
    "None"
  };
  items_[AntiAliasing_Normal] = {
    FT_LOAD_TARGET_NORMAL,
    "Normal"
  };
  items_[AntiAliasing_Light] = {
    FT_LOAD_TARGET_LIGHT,
    "Light"
  };
  items_[AntiAliasing_LCD] = {
    FT_LOAD_TARGET_LCD,
    "LCD (RGB)"
  };
  items_[AntiAliasing_LCD_BGR] = {
    FT_LOAD_TARGET_LCD,
    "LCD (BGR)"
  };
  items_[AntiAliasing_LCD_Vertical] = {
    FT_LOAD_TARGET_LCD_V,
    "LCD (vert. RGB)"
  };
  items_[AntiAliasing_LCD_Vertical_BGR] = {
    FT_LOAD_TARGET_LCD_V, // XXX Bug: No difference between RGB and BGR?
    "LCD (vert. BGR)"
  };

  lightAntiAliasingEnabled_ = true;
}


QVariant
AntiAliasingComboBoxModel::data(const QModelIndex& index,
                                int role) const
{
  if (role == Qt::ForegroundRole)
    if (index.row() == AntiAliasing_Light && !lightAntiAliasingEnabled_)
      return QApplication::palette().color(QPalette::Disabled, 
                                           QPalette::Text);
  return SimpleComboBoxModel::data(index, role);
}


Qt::ItemFlags
AntiAliasingComboBoxModel::flags(const QModelIndex& index) const
{
  if (index.row() == AntiAliasing_Light && !lightAntiAliasingEnabled_)
    return Qt::ItemFlags {};
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}


// end of ttsettingscomboboxmodel.cpp
