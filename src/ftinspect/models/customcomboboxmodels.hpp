// customcomboboxmodels.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.


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
  ~HintingModeComboBoxModel() override = default;

  int rowCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index,
                int role) const override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  int indexToTTInterpreterVersion(int index) const;
  int indexToCFFMode(int index) const;
  int cffModeToIndex(int mode) const;
  int ttInterpreterVersionToIndex(int version) const;

  void setSupportedModes(QList<int> supportedTTIVersions,
                         QList<int> supportedCFFModes);
  void setCurrentEngineType(HintingEngineType type, bool tricky);

private:
  QHash<int, HintingModeItem> items_;

  int indexToValueForType(int index,
                          HintingEngineType type) const;
  int valueToIndexForType(int value,
                          HintingEngineType type) const;

public:
  // Note: Ensure related funcs are also updated when
  //       these enums are changed!
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


// A simple key-displayName-value model for `QComboBox`.
template <class T>
class SimpleComboBoxModelImpl
{
public:
  struct ComboBoxItem
  {
    T value;
    QString displayName;
  };

  SimpleComboBoxModelImpl() {}
  virtual ~SimpleComboBoxModelImpl() = default;

  virtual int
  rowCount(const QModelIndex& parent) const
  {
    return items_.size();
  }


  virtual QVariant
  data(const QModelIndex& index,
       int role) const
  {
    if (role != Qt::DisplayRole)
      return QVariant{};

    int r = index.row();
    if (r < 0 || r >= items_.size())
      return QVariant{};
    return items_[r].displayName;
  }


  virtual T
  indexToValue(int index)
  {
    if (index < 0 || index >= items_.size())
      return T();
    return items_[index].value;
  }

protected:
  QHash<int, ComboBoxItem> items_;
};


class LCDFilterComboBoxModel
: public QAbstractListModel,
  public SimpleComboBoxModelImpl<int>
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
  ~LCDFilterComboBoxModel() override = default;

  int rowCount(const QModelIndex& parent) const override
        { return SimpleComboBoxModelImpl::rowCount(parent); }


  QVariant data(const QModelIndex& index,
                int role) const override
             { return SimpleComboBoxModelImpl::data(index, role); }

public:
  enum LCDFilter : int
  {
    LCDFilter_Default,
    LCDFilter_Light,
    LCDFilter_None,
    LCDFilter_Legacy
  };
};


struct AASetting
{
  // No default value for braced init - No C++14, what a pain!
  int loadFlag;
  int renderMode;
  bool isBGR;
};


class AntiAliasingComboBoxModel
: public QAbstractListModel,
  public SimpleComboBoxModelImpl<AASetting>
{
  Q_OBJECT

public:
  enum AntiAliasing : int;

  explicit AntiAliasingComboBoxModel(QObject* parent);
  ~AntiAliasingComboBoxModel() override = default;

  QVariant data(const QModelIndex& index,
                int role) const;
  Qt::ItemFlags flags(const QModelIndex& index) const;

  int rowCount(const QModelIndex& parent) const override
        { return SimpleComboBoxModelImpl::rowCount(parent); }

  void setLightAntiAliasingEnabled(bool enabled)
         { lightAntiAliasingEnabled_ = enabled; }

private:
  bool lightAntiAliasingEnabled_;

public:
  enum AntiAliasing : int
  {
    AntiAliasing_None,
    AntiAliasing_Normal,
    AntiAliasing_Light,
    AntiAliasing_Light_SubPixel,
    AntiAliasing_LCD,
    AntiAliasing_LCD_BGR,
    AntiAliasing_LCD_Vertical,
    AntiAliasing_LCD_Vertical_BGR
  };
};


// end of customcomboboxmodels.hpp
