// fontinfomodels.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include "../engine/fontinfo.hpp"
#include "../engine/charmap.hpp"
#include "../engine/mmgx.hpp"

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
    FSIM_Height = 0,
    FSIM_Width,
    FSIM_Size,
    FSIM_XPpem,
    FSIM_YPpem,
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
    CMIM_Platform   = 0,
    CMIM_Encoding,
    CMIM_FormatID,
    CMIM_Language,
    CMIM_MaxIndex,
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
    SNM_Name       = 0,
    SNM_Platform,
    SNM_Encoding,
    SNM_Language,
    SNM_Content,
    SNM_Max
  };

private:
  // Don't let the item count exceed INT_MAX!
  std::vector<SFNTName> storage_;
};


class SFNTTableInfoModel
: public QAbstractTableModel
{
  Q_OBJECT
public:
  explicit SFNTTableInfoModel(QObject* parent) : QAbstractTableModel(parent) {}
  ~SFNTTableInfoModel() override = default;

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
  std::vector<SFNTTableInfo>& storage() { return storage_; }

  enum Columns : int
  {
    STIM_Tag = 0,
    STIM_Offset,
    STIM_Length,
    STIM_Valid,
    STIM_SharedFaces,
    STIM_Max
  };

private:
  // Don't let the item count exceed INT_MAX!
  std::vector<SFNTTableInfo> storage_;
};


class MMGXAxisInfoModel
: public QAbstractTableModel
{
  Q_OBJECT
public:
  explicit MMGXAxisInfoModel(QObject* parent) : QAbstractTableModel(parent) {}
  ~MMGXAxisInfoModel() override = default;

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
  std::vector<MMGXAxisInfo>& storage() { return storage_; }

  enum Columns : int
  {
    MAIM_Tag = 0,
    MAIM_Minimum,
    MAIM_Default,
    MAIM_Maximum,
    MAIM_Hidden,
    MAIM_Name,
    MAIM_Max
  };

private:
  // Don't let the item count exceed INT_MAX!
  std::vector<MMGXAxisInfo> storage_;
};


// end of fontinfomodels.hpp
