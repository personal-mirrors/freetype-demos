// info.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once

#include "abstracttab.hpp"
#include "../engine/fontinfo.hpp"
#include "../models/fontinfomodels.hpp"
#include "../widgets/customwidgets.hpp"

#include <vector>

#include <QBoxLayout>
#include <QDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTableView>
#include <QTabWidget>
#include <QTextEdit>
#include <QTreeView>
#include <QVector>
#include <QWidget>

class Engine;
class GeneralInfoTab;
class SFNTInfoTab;
class PostScriptInfoTab;
class MMGXInfoTab;
class CompositeGlyphsTab;

class InfoTab
: public QWidget,
  public AbstractTab
{
  Q_OBJECT

public:
  InfoTab(QWidget* parent,
          Engine* engine);
  ~InfoTab() override = default;

  void repaintGlyph() override {}
  void reloadFont() override;

signals:
  void switchToSingular(int glyphIndex);

private:
  Engine* engine_;

  QVector<AbstractTab*> tabs_;
  GeneralInfoTab* generalTab_;
  SFNTInfoTab* sfntTab_;
  PostScriptInfoTab* postScriptTab_;
  MMGXInfoTab* mmgxTab_;
  CompositeGlyphsTab* compositeGlyphsTab_;

  QTabWidget* tab_;
  QHBoxLayout* layout_;

  void createLayout();
  void createConnections();
};


#define LabelPair(name) \
  QLabel* name##Label_; \
  QLabel* name##PromptLabel_;


class GeneralInfoTab
: public QWidget,
  public AbstractTab
{
  Q_OBJECT

public:
  GeneralInfoTab(QWidget* parent,
                 Engine* engine);
  ~GeneralInfoTab() override = default;

  void repaintGlyph() override {}
  void reloadFont() override;

private:
  Engine* engine_;

  LabelPair(numFaces)
  LabelPair(family)
  LabelPair(style)
  LabelPair(postscript)
  LabelPair(created)
  LabelPair(modified)
  LabelPair(revision)
  LabelPair(copyright)
  LabelPair(trademark)
  LabelPair(manufacturer)

  LabelPair(driverName)
  LabelPair(sfnt)
  LabelPair(fontType)
  LabelPair(direction)
  LabelPair(fixedWidth)
  LabelPair(glyphNames)

  LabelPair(emSize)
  LabelPair(bbox)
  LabelPair(ascender)
  LabelPair(descender)
  LabelPair(maxAdvanceWidth)
  LabelPair(maxAdvanceHeight)
  LabelPair(ulPos)
  LabelPair(ulThickness)

  QGroupBox* basicGroupBox_;
  QGroupBox* typeEntriesGroupBox_;
  QGroupBox* charMapGroupBox_;
  QGroupBox* fixedSizesGroupBox_;

  QTableView* charMapsTable_;
  QTableView* fixedSizesTable_;

  FixedSizeInfoModel* fixedSizeInfoModel_;
  CharMapInfoModel* charMapInfoModel_;

  UnboundScrollArea* leftScrollArea_;

  QWidget* leftWidget_;
  QHBoxLayout* mainLayout_;
  QVBoxLayout* leftLayout_;
  QVBoxLayout* rightLayout_;
  QGridLayout* basicLayout_;
  QGridLayout* typeEntriesLayout_;
  QHBoxLayout* charMapLayout_;
  QHBoxLayout* fixedSizesLayout_;

  std::vector<QLabel*> scalableOnlyLabels_;

  FontBasicInfo oldFontBasicInfo_ = {};
  FontTypeEntries oldFontTypeEntries_ = {};

  void createLayout();
};


class StringViewDialog
: public QDialog
{
  Q_OBJECT

public:
  StringViewDialog(QWidget* parent);
  ~StringViewDialog() override = default;

  void updateString(QByteArray const& rawArray,
                    QString const& str);

private:
  QLabel* textLabel_;
  QLabel* hexTextLabel_;

  QTextEdit* textEdit_;
  QTextEdit* hexTextEdit_;

  QVBoxLayout* layout_;

  void createLayout();
};


class SFNTInfoTab
: public QWidget,
  public AbstractTab
{
  Q_OBJECT

public:
  SFNTInfoTab(QWidget* parent,
              Engine* engine);
  ~SFNTInfoTab() override = default;

  void repaintGlyph() override {}
  void reloadFont() override;

private:
  Engine* engine_;

  QGroupBox* sfntNamesGroupBox_;
  QGroupBox* sfntTablesGroupBox_;

  QTableView* sfntNamesTable_;
  QTableView* sfntTablesTable_;

  SFNTNameModel* sfntNamesModel_;
  SFNTTableInfoModel* sfntTablesModel_;

  QHBoxLayout* sfntNamesLayout_;
  QHBoxLayout* sfntTablesLayout_;
  QHBoxLayout* mainLayout_;

  StringViewDialog* stringViewDialog_;

  void createLayout();
  void createConnections();

  void nameTableDoubleClicked(QModelIndex const& index);
};


class PostScriptInfoTab
: public QWidget,
  public AbstractTab
{
  Q_OBJECT

public:
  PostScriptInfoTab(QWidget* parent,
                    Engine* engine);
  ~PostScriptInfoTab() override = default;

  void repaintGlyph() override {}
  void reloadFont() override;

private:
  Engine* engine_;

  LabelPair(version)
  LabelPair(notice)
  LabelPair(fullName)
  LabelPair(familyName)
  LabelPair(weight)
  LabelPair(italicAngle)
  LabelPair(fixedPitch)
  LabelPair(ulPos)
  LabelPair(ulThickness)

  LabelPair(uniqueID)
  LabelPair(blueValues)
  LabelPair(otherBlues)
  LabelPair(familyBlues)
  LabelPair(familyOtherBlues)
  LabelPair(blueScale)
  LabelPair(blueShift)
  LabelPair(blueFuzz)
  LabelPair(stdWidths)
  LabelPair(stdHeights)
  LabelPair(snapWidths)
  LabelPair(snapHeights)
  LabelPair(forceBold)
  LabelPair(languageGroup)
  LabelPair(password)
  LabelPair(lenIV)
  LabelPair(minFeature)
  LabelPair(roundStemUp)
  LabelPair(expansionFactor)

  QGroupBox* infoGroupBox_;
  QGroupBox* privateGroupBox_;

  QWidget* infoWidget_;
  QWidget* privateWidget_;

  UnboundScrollArea* infoScrollArea_;
  UnboundScrollArea* privateScrollArea_;

  QGridLayout* infoLayout_;
  QGridLayout* privateLayout_;
  QHBoxLayout* infoGroupBoxLayout_;
  QHBoxLayout* privateGroupBoxLayout_;
  QHBoxLayout* mainLayout_;

  PS_PrivateRec oldFontPrivate_;

  void createLayout();
};


class MMGXInfoTab
: public QWidget,
  public AbstractTab
{
  Q_OBJECT
public:
  MMGXInfoTab(QWidget* parent,
              Engine* engine);
  ~MMGXInfoTab() override = default;

  void repaintGlyph() override {}
  void reloadFont() override;

private:
  Engine* engine_;

  LabelPair(mmgxType)

  QGroupBox* axesGroupBox_;
  QTableView* axesTable_;

  QGridLayout* infoLayout_;
  QHBoxLayout* axesLayout_;
  QVBoxLayout* mainLayout_;

  MMGXAxisInfoModel* axesModel_;

  void createLayout();
};


class CompositeGlyphsTab
: public QWidget,
  public AbstractTab
{
  Q_OBJECT

public:
  CompositeGlyphsTab(QWidget* parent,
                     Engine* engine);
  ~CompositeGlyphsTab() override = default;

  void repaintGlyph() override {}
  void reloadFont() override;

signals:
  void switchToSingular(int glyphIndex);

private:
  Engine* engine_;

  LabelPair(compositeGlyphCount)
  QPushButton* forceRefreshButton_;
  QTreeView* compositeTreeView_;
  CompositeGlyphsInfoModel* compositeModel_;

  QHBoxLayout* countLayout_;
  QVBoxLayout* mainLayout_;

  void createLayout();
  void createConnections();

  void forceReloadFont();
  void treeRowDoubleClicked(const QModelIndex& idx);
};


// end of info.hpp
