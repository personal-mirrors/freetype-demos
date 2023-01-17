// fontinfomodels.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once

#include "../engine/charmap.hpp"
#include "../engine/fontinfo.hpp"
#include "../engine/mmgx.hpp"

#include <unordered_map>
#include <vector>

#include <QAbstractTableModel>
#include <QPixmap>


class FixedSizeInfoModel
: public QAbstractTableModel
{
  Q_OBJECT

public:
  explicit FixedSizeInfoModel(QObject* parent)
           : QAbstractTableModel(parent) {}
  ~FixedSizeInfoModel() override = default;

  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index,
                int role) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role) const override;

  // Since we need to call `beginResetModel` right before updating, we need
  // to call `endResetModel` after the internal storage is changed.  The
  // model should be updated on-demand, and the internal storage is updated
  // from `FontFixedSize::get`.  We provide a callback for `get` to ensure
  // the `beginResetModel` is called before the storage is changed, and the
  // caller is responsible to call `endResetModel` according to `get`'s
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
  explicit CharMapInfoModel(QObject* parent)
           : QAbstractTableModel(parent) {}
  ~CharMapInfoModel() override = default;

  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index,
                int role) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role) const override;

  // The same as `FixedSizeInfoModel`.
  void beginModelUpdate() { beginResetModel(); }
  void endModelUpdate() { endResetModel(); }
  std::vector<CharMapInfo>& storage() { return storage_; }

  enum Columns : int
  {
    CMIM_Platform = 0,
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
  explicit SFNTNameModel(QObject* parent)
           : QAbstractTableModel(parent) {}
  ~SFNTNameModel() override = default;

  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index,
                int role) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role) const override;

  // The same as `FixedSizeInfoModel`.
  void beginModelUpdate() { beginResetModel(); }
  void endModelUpdate() { endResetModel(); }
  std::vector<SFNTName>& storage() { return storage_; }

  enum Columns : int
  {
    SNM_Name = 0,
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
  explicit SFNTTableInfoModel(QObject* parent)
           : QAbstractTableModel(parent) {}
  ~SFNTTableInfoModel() override = default;

  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index,
                int role) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role) const override;

  // The same as `FixedSizeInfoModel`.
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
  explicit MMGXAxisInfoModel(QObject* parent)
           : QAbstractTableModel(parent) {}
  ~MMGXAxisInfoModel() override = default;

  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index,
                int role) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role) const override;

  // The same as `FixedSizeInfoModel`.
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


struct LookupPairHash
{
public:
  std::size_t
  operator()(const std::pair<int, long long>& p) const
  {
    std::size_t seed = 0x291FEEA8;
    seed ^= (seed << 6) + (seed >> 2) + 0x25F3E86D
            + static_cast<std::size_t>(p.first);
    seed ^= (seed << 6) + (seed >> 2) + 0x436E6B92
            + static_cast<std::size_t>(p.second);
    return seed;
  }
};


// A tree model, so much more complicated.
class CompositeGlyphsInfoModel
: public QAbstractItemModel
{
  Q_OBJECT

public:
  // A lazily created info node.
  struct InfoNode
  {
    long long parentNodeIndex;
    int indexInParent;
    int glyphIndex;
    ptrdiff_t glyphInfoIndex;
    CompositeGlyphInfo::SubGlyph const* subGlyphInfo;
  };

  explicit CompositeGlyphsInfoModel(QObject* parent,
                                    Engine* engine)
           : QAbstractItemModel(parent),
             engine_(engine)
  {
  }

  ~CompositeGlyphsInfoModel() override = default;

  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override;
  QModelIndex index(int row,
                    int column,
                    const QModelIndex& parent) const override;
  QModelIndex parent(const QModelIndex& child) const override;
  QVariant data(const QModelIndex& index,
                int role) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role) const override;
  int glyphIndexFromIndex(const QModelIndex& idx);

  void beginModelUpdate();
  void endModelUpdate();
  std::vector<CompositeGlyphInfo>& storage() { return glyphs_; }

  enum Columns : int
  {
    CGIM_Glyph = 0,
    CGIM_Flag = 1,
    CGIM_PositionTransformation = 2,
    CGIM_Max
  };

private:
  Engine* engine_;
  // Take care of 3 types of indices:
  //
  // 1. the glyph index in a font file
  // 2. the glyph index in `glyphs_` - often called 'glyph info index'
  // 3. a node index
  std::vector<CompositeGlyphInfo> glyphs_;

  // Glyph index -> glyph info index.
  std::unordered_map<int, size_t> glyphMapper_;
  // Map <row, parentId> to node.
  // The internal ID of `QModelIndex` is the node's index.
  mutable std::unordered_map<std::pair<int, long long>,
                             long long, LookupPairHash>
          nodeLookup_;
  mutable std::vector<InfoNode> nodes_;

  mutable std::unordered_map<int, QPixmap> glyphIcons_;

  // This has to be const.
  QPixmap renderIcon(int glyphIndex) const;
};


// end of fontinfomodels.hpp
