// ttsettingscomboboxmodel.hpp

// Copyright (C) 2022 by Charlie Jiang.


#pragma once

#include <QAbstractListModel>
#include <QHash>

class HintingModeComboBoxModel
: public QAbstractListModel
{
  Q_OBJECT
public:
  enum HintingEngineType : int;
  enum HintingMode : int;
  struct HintingModeItem
  {
    HintingEngineType type;
    HintingMode key;
    int value;
    bool enabled;
    bool supported;
    QString displayName;
  };

  explicit HintingModeComboBoxModel(QObject* parent);
  virtual ~HintingModeComboBoxModel() = default;

  int rowCount(const QModelIndex& parent) const;
  QVariant data(const QModelIndex& index,
                int role) const;
  Qt::ItemFlags flags(const QModelIndex& index) const;

  int indexToTTInterpreterVersion(int index) const;
  int indexToCFFMode(int index) const;
  int cffModeToIndex(int mode) const;
  int ttInterpreterVersionToIndex(int version) const;

  void setSupportedModes(QList<int> supportedTTIVersions,
                         QList<int> supportedCFFModes);
  void setCurrentEngineType(HintingEngineType type);

private:
  QHash<int, HintingModeItem> items_;

  int indexToValueForType(int index,
                          HintingEngineType type) const;
  int valueToIndexForType(int value,
                          HintingEngineType type) const;

public:
  // Note: Ensure related funcs are also changed when
  // these enums are changed!
  enum HintingEngineType : int
  {
    HintingEngineType_TrueType,
    HintingEngineType_CFF
  };

  enum HintingMode : int
  {
    HintingMode_TrueType_v35 = 0,
    HintingMode_TrueType_v38,
    HintingMode_TrueType_v40,
    HintingMode_CFF_FreeType,
    HintingMode_CFF_Adobe
  };
};


// A simple key-displayName-value model for QComboBox.
class SimpleComboBoxModel
: public QAbstractListModel
{
  Q_OBJECT
public:
  struct ComboBoxItem
  {
    int value;
    QString displayName;
  };

  explicit SimpleComboBoxModel(QObject* parent);
  virtual ~SimpleComboBoxModel() = default;

  int rowCount(const QModelIndex& parent) const;
  QVariant data(const QModelIndex& index,
                int role) const;

  int indexToValue(int index);

protected:
  QHash<int, ComboBoxItem> items_;
};


class LCDFilterComboBoxModel
: public SimpleComboBoxModel
{
  Q_OBJECT
public:
  enum LCDFilter : int;
  struct LCDFilterItem
  {
    int value;
    QString displayName;
  };

  explicit LCDFilterComboBoxModel(QObject* parent);
  virtual ~LCDFilterComboBoxModel() = default;

public:
  enum LCDFilter
  {
    LCDFilter_Default,
    LCDFilter_Light,
    LCDFilter_None,
    LCDFilter_Legacy
  };
};


class AntiAliasingComboBoxModel
: public SimpleComboBoxModel
{
  Q_OBJECT
public:
  enum AntiAliasing : int;

  explicit AntiAliasingComboBoxModel(QObject* parent);
  virtual ~AntiAliasingComboBoxModel() = default;
  
  QVariant data(const QModelIndex& index,
                int role) const;
  Qt::ItemFlags flags(const QModelIndex& index) const;

  void setLightAntiAliasingEnabled(bool enabled)
  {
    lightAntiAliasingEnabled_ = enabled;
  }

private:
  bool lightAntiAliasingEnabled_;

public:
  enum AntiAliasing : int
  {
    AntiAliasing_None,
    AntiAliasing_Normal,
    AntiAliasing_Light,
    AntiAliasing_LCD,
    AntiAliasing_LCD_BGR,
    AntiAliasing_LCD_Vertical,
    AntiAliasing_LCD_Vertical_BGR
  };
};


// end of ttsettingscomboboxmodel.hpp
