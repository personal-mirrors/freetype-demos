// info.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include "abstracttab.hpp"
#include "../engine/fontinfo.hpp"
#include "../models/fontinfomodels.hpp"

#include <vector>
#include <QWidget>
#include <QTabWidget>
#include <QBoxLayout>
#include <QTextEdit>
#include <QDialog>
#include <QGridLayout>
#include <QVector>
#include <QLabel>
#include <QGroupBox>
#include <QTableView>
#include <QStackedLayout>

class Engine;
class GeneralInfoTab;
class SFNTInfoTab;
class PostScriptInfoTab;
class MMGXInfoTab;

class InfoTab
: public QWidget, public AbstractTab
{
  Q_OBJECT
public:
  InfoTab(QWidget* parent, Engine* engine);
  ~InfoTab() override = default;

  void repaintGlyph() override {}
  void reloadFont() override;

private:
  Engine* engine_;

  QVector<AbstractTab*> tabs_;
  GeneralInfoTab*    generalTab_;
  SFNTInfoTab*       sfntTab_;
  PostScriptInfoTab* postScriptTab_;
  MMGXInfoTab*       mmgxTab_;

  QTabWidget* tab_;
  QHBoxLayout* layout_;

  void createLayout();
};


class GeneralInfoTab
: public QWidget, public AbstractTab
{
  Q_OBJECT
public:
  GeneralInfoTab(QWidget* parent, Engine* engine);
  ~GeneralInfoTab() override = default;

  void repaintGlyph() override {}
  void reloadFont() override;

private:
  Engine* engine_;

#define LabelPair(name) \
  QLabel* name##Label_; \
  QLabel* name##PromptLabel_;

  LabelPair(    numFaces)
  LabelPair(      family)
  LabelPair(       style)
  LabelPair(  postscript)
  LabelPair(     created)
  LabelPair(    modified)
  LabelPair(    revision)
  LabelPair(   copyright)
  LabelPair(   trademark)
  LabelPair(manufacturer)

  LabelPair(driverName)
  LabelPair(      sfnt)
  LabelPair(  fontType)
  LabelPair( direction)
  LabelPair(fixedWidth)
  LabelPair(glyphNames)

  LabelPair(          emSize)
  LabelPair(            bbox)
  LabelPair(        ascender)
  LabelPair(       descender)
  LabelPair( maxAdvanceWidth)
  LabelPair(maxAdvanceHeight)
  LabelPair(           ulPos)
  LabelPair(     ulThickness)

  QGroupBox*       basicGroupBox_;
  QGroupBox* typeEntriesGroupBox_;
  QGroupBox*     charMapGroupBox_;
  QGroupBox*  fixedSizesGroupBox_;

  QTableView*   charMapsTable_;
  QTableView* fixedSizesTable_;

  FixedSizeInfoModel* fixedSizeInfoModel_;
  CharMapInfoModel* charMapInfoModel_;

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

  void updateString(QByteArray const& rawArray, QString const& str);

private:
  QLabel* textLabel_;
  QLabel* hexTextLabel_;

  QTextEdit* textEdit_;
  QTextEdit* hexTextEdit_;

  QVBoxLayout* layout_;

  void createLayout();
};


class SFNTInfoTab
: public QWidget, public AbstractTab
{
  Q_OBJECT
public:
  SFNTInfoTab(QWidget* parent, Engine* engine);
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

  QHBoxLayout* sfntNamesLayout_;
  QHBoxLayout* sfntTablesLayout_;
  QHBoxLayout* mainLayout_;

  StringViewDialog* stringViewDialog_;

  void createLayout();
  void createConnections();

  void nameTableDoubleClicked(QModelIndex const& index);
};


class PostScriptInfoTab
: public QWidget, public AbstractTab
{
  Q_OBJECT
public:
  PostScriptInfoTab(QWidget* parent, Engine* engine);
  ~PostScriptInfoTab() override = default;

  void repaintGlyph() override {}
  void reloadFont() override;

private:
  Engine* engine_;
};


class MMGXInfoTab
: public QWidget, public AbstractTab
{
  Q_OBJECT
public:
  MMGXInfoTab(QWidget* parent, Engine* engine);
  ~MMGXInfoTab() override = default;

  void repaintGlyph() override {}
  void reloadFont() override;

private:
  Engine* engine_;
};


// end of info.hpp
