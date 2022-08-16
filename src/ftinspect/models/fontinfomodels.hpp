// fontinfomodels.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include "../engine/fontinfo.hpp"
#include "../engine/charmap.hpp"

#include <vector>
#include <QAbstractTableModel>

class FixedSizeInfoModel
: public QAbstractTableModel
{
  Q_OBJECT
public:
  explicit FixedSizeInfoModel(QObject* parent) : QAbstractTableModel(parent) {}
  ~FixedSizeInfoModel() override = default;

  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index,
                int role) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role) const override;

  // Since we need to call `beginResetModel` right before updating, and need to
  // call `endResetModel` after the internal storage is changed
  // The model should be updated on-demand, and the internal storage is updated
  // from `FontFixedSize::get`, we provide a callback for `get` to ensure the
  // `beginResetModel` is called before the storage is changed,
  // and the caller is responsible to call `endResetModel` according to `get`'s
  // return value.
  void beginModelUpdate() { beginResetModel(); }
  void endModelUpdate() { endResetModel(); }
  std::vector<FontFixedSize>& storage() { return storage_; }

  enum Columns : int
  {
    FSIM_Index  = 0,
    FSIM_Height = 1,
    FSIM_Width  = 2,
    FSIM_Size   = 3,
    FSIM_XPpem  = 4,
    FSIM_YPpem  = 5,
    FSIM_Max
  };

private:
  // Don't let the item count exceed INT_MAX!
  std::vector<FontFixedSize> storage_;
};


class CharMapInfoModel
: public QAbstractTableModel
{
  Q_OBJECT
public:
  explicit CharMapInfoModel(QObject* parent) : QAbstractTableModel(parent) {}
  ~CharMapInfoModel() override = default;

  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index,
                int role) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role) const override;

  // Same to `FixedSizeInfoModel`
  void beginModelUpdate() { beginResetModel(); }
  void endModelUpdate() { endResetModel(); }
  std::vector<CharMapInfo>& storage() { return storage_; }

  enum Columns : int
  {
    CMIM_Index      = 0,
    CMIM_Platform   = 1,
    CMIM_Encoding   = 2,
    CMIM_FormatID   = 3,
    CMIM_Language   = 4,
    CMIM_MaxIndex   = 5,
    CMIM_Max
  };

private:
  // Don't let the item count exceed INT_MAX!
  std::vector<CharMapInfo> storage_;
};


class SFNTNameModel
: public QAbstractTableModel
{
  Q_OBJECT
public:
  explicit SFNTNameModel(QObject* parent) : QAbstractTableModel(parent) {}
  ~SFNTNameModel() override = default;

  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index,
                int role) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role) const override;

  // Same to `FixedSizeInfoModel`
  void beginModelUpdate() { beginResetModel(); }
  void endModelUpdate() { endResetModel(); }
  std::vector<SFNTName>& storage() { return storage_; }

  enum Columns : int
  {
    SNM_Index      = 0,
    SNM_Name       = 1,
    SNM_Platform   = 2,
    SNM_Encoding   = 3,
    SNM_Language   = 4,
    SNM_Content    = 5,
    SNM_Max
  };

private:
  // Don't let the item count exceed INT_MAX!
  std::vector<SFNTName> storage_;
};


// end of fontinfomodels.hpp
