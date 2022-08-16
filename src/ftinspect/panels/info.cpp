// info.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "info.hpp"

#include "../uihelper.hpp"
#include "../engine/engine.hpp"

#include <QStringList>
#include <QHeaderView>

InfoTab::InfoTab(QWidget* parent,
                 Engine* engine)
: QWidget(parent), engine_(engine)
{
  createLayout();
}


void
InfoTab::reloadFont()
{
  for (auto tab : tabs_)
    tab->reloadFont();
}


void
InfoTab::createLayout()
{
  generalTab_ = new GeneralInfoTab(this, engine_);
  sfntTab_ = new SFNTInfoTab(this, engine_);
  postScriptTab_ = new PostScriptInfoTab(this, engine_);
  mmgxTab_ = new MMGXInfoTab(this, engine_);

  tab_ = new QTabWidget(this);
  tab_->addTab(generalTab_, tr("General"));
  tab_->addTab(sfntTab_, tr("SFNT"));
  tab_->addTab(postScriptTab_, tr("PostScript"));
  tab_->addTab(mmgxTab_, tr("MM/GX"));

  tabs_.append(generalTab_);
  tabs_.append(sfntTab_);
  tabs_.append(postScriptTab_);
  tabs_.append(mmgxTab_);

  layout_ = new QHBoxLayout;
  layout_->addWidget(tab_);

  setLayout(layout_);
}


GeneralInfoTab::GeneralInfoTab(QWidget* parent,
                               Engine* engine)
: QWidget(parent), engine_(engine)
{
  createLayout();
}


void
GeneralInfoTab::reloadFont()
{
  auto basicInfo = FontBasicInfo::get(engine_);
  // don't update when unnecessary
  if (basicInfo != oldFontBasicInfo_) 
  {
    oldFontBasicInfo_ = basicInfo;
    if (basicInfo.numFaces < 0)
      numFacesLabel_->clear();
    else
      numFacesLabel_->setText(QString::number(basicInfo.numFaces));
    
          familyLabel_->setText(basicInfo.familyName);
           styleLabel_->setText(basicInfo.styleName);
      postscriptLabel_->setText(basicInfo.postscriptName);
        revisionLabel_->setText(basicInfo.revision);
       copyrightLabel_->setText(basicInfo.copyright);
       trademarkLabel_->setText(basicInfo.trademark);
    manufacturerLabel_->setText(basicInfo.manufacturer);
    
    createdLabel_->setText(
        basicInfo.createdTime.toString("yyyy-MM-dd hh:mm:ss t"));
    modifiedLabel_->setText(
        basicInfo.modifiedTime.toString("yyyy-MM-dd hh:mm:ss t"));
  }

  auto fontTypeEntries = FontTypeEntries::get(engine_);

  if (fontTypeEntries != oldFontTypeEntries_)
  {
    oldFontTypeEntries_ = fontTypeEntries;
    QString directionText;
    // Don't want to do concat...
    if (fontTypeEntries.hasHorizontal && fontTypeEntries.hasVertical)
      directionText = "honizontal, vertical";
    else if (fontTypeEntries.hasHorizontal)
      directionText = "horizontal";
    else if (fontTypeEntries.hasVertical)
      directionText = "vertical";
    
    QStringList types;
    if (fontTypeEntries.scalable)
      types += "scalable";
    if (fontTypeEntries.mmgx)
      types += "multiple master";
    if (fontTypeEntries.fixedSizes)
      types += "fixed sizes";
    
    driverNameLabel_->setText(fontTypeEntries.driverName);
          sfntLabel_->setText(fontTypeEntries.sfnt ? "yes" : "no");
      fontTypeLabel_->setText(types.join(", "));
     directionLabel_->setText(directionText);
    fixedWidthLabel_->setText(fontTypeEntries.fixedWidth ? "yes" : "no");
    glyphNamesLabel_->setText(fontTypeEntries.glyphNames ? "available"
                                                           : "unavailable");

    if (fontTypeEntries.scalable)
    {
      emSizeLabel_->setText(QString::number(fontTypeEntries.emSize));
      bboxLabel_->setText(QString("(%1, %2) : (%3, %4)")
                            .arg(fontTypeEntries.globalBBox.xMin)
                            .arg(fontTypeEntries.globalBBox.yMin)
                            .arg(fontTypeEntries.globalBBox.xMax)
                            .arg(fontTypeEntries.globalBBox.yMax));
      ascenderLabel_->setText(QString::number(fontTypeEntries.ascender));
      descenderLabel_->setText(QString::number(fontTypeEntries.descender));
      maxAdvanceWidthLabel_
        ->setText(QString::number(fontTypeEntries.maxAdvanceWidth));
      maxAdvanceHeightLabel_
        ->setText(QString::number(fontTypeEntries.maxAdvanceHeight));
      ulPosLabel_
        ->setText(QString::number(fontTypeEntries.underlinePos));
      ulThicknessLabel_
        ->setText(QString::number(fontTypeEntries.underlineThickness));

      for (auto label : scalableOnlyLabels_)
        label->setEnabled(true);
    }
    else
    {
      for (auto label : scalableOnlyLabels_)
        label->setEnabled(false);
    }
  }

  fixedSizesTable_->setEnabled(fontTypeEntries.fixedSizes);
  bool reset
    = FontFixedSize::get(engine_, 
                         fixedSizeInfoModel_->storage(),
                         [&] { fixedSizeInfoModel_->beginModelUpdate(); });
  if (reset)
    fixedSizeInfoModel_->endModelUpdate();

  if (engine_->currentFontCharMaps() != charMapInfoModel_->storage())
  {
    charMapInfoModel_->beginModelUpdate();
    charMapInfoModel_->storage() = engine_->currentFontCharMaps();
    charMapInfoModel_->endModelUpdate();
  }
}


void
GeneralInfoTab::createLayout()
{
      numFacesPromptLabel_  = new QLabel(tr("Num of Faces:"), this);
        familyPromptLabel_  = new QLabel(tr("Family Name:"), this);
         stylePromptLabel_  = new QLabel(tr("Style Name:"), this);
    postscriptPromptLabel_  = new QLabel(tr("PostScript Name:"), this);
       createdPromptLabel_  = new QLabel(tr("Created at:"), this);
      modifiedPromptLabel_  = new QLabel(tr("Modified at:"), this);
      revisionPromptLabel_  = new QLabel(tr("Font Revision:"), this);
     copyrightPromptLabel_  = new QLabel(tr("Copyright:"), this);
     trademarkPromptLabel_  = new QLabel(tr("Trademark:"), this);
  manufacturerPromptLabel_  = new QLabel(tr("Manufacturer:"), this);

      numFacesLabel_ = new QLabel(this);
        familyLabel_ = new QLabel(this);
         styleLabel_ = new QLabel(this);
    postscriptLabel_ = new QLabel(this);
       createdLabel_ = new QLabel(this);
      modifiedLabel_ = new QLabel(this);
      revisionLabel_ = new QLabel(this);
     copyrightLabel_ = new QLabel(this);
     trademarkLabel_ = new QLabel(this);
  manufacturerLabel_ = new QLabel(this);

  setLabelSelectable(    numFacesLabel_);
  setLabelSelectable(      familyLabel_);
  setLabelSelectable(       styleLabel_);
  setLabelSelectable(  postscriptLabel_);
  setLabelSelectable(     createdLabel_);
  setLabelSelectable(    modifiedLabel_);
  setLabelSelectable(    revisionLabel_);
  setLabelSelectable(   copyrightLabel_);
  setLabelSelectable(   trademarkLabel_);
  setLabelSelectable(manufacturerLabel_);

  driverNamePromptLabel_ = new QLabel(tr("Driver:"), this);
        sfntPromptLabel_ = new QLabel(tr("SFNT Wrapped:"), this);
    fontTypePromptLabel_ = new QLabel(tr("Type:"), this);
   directionPromptLabel_ = new QLabel(tr("Direction:"), this);
  fixedWidthPromptLabel_ = new QLabel(tr("Fixed Width:"), this);
  glyphNamesPromptLabel_ = new QLabel(tr("Glyph Names:"), this);

  driverNameLabel_ = new QLabel(this);
        sfntLabel_ = new QLabel(this);
    fontTypeLabel_ = new QLabel(this);
   directionLabel_ = new QLabel(this);
  fixedWidthLabel_ = new QLabel(this);
  glyphNamesLabel_ = new QLabel(this);

  setLabelSelectable(driverNameLabel_);
  setLabelSelectable(      sfntLabel_);
  setLabelSelectable(  fontTypeLabel_);
  setLabelSelectable( directionLabel_);
  setLabelSelectable(fixedWidthLabel_);
  setLabelSelectable(glyphNamesLabel_);

            emSizePromptLabel_ = new QLabel(tr("EM Size:"), this);
              bboxPromptLabel_ = new QLabel(tr("Global BBox:"), this);
          ascenderPromptLabel_ = new QLabel(tr("Ascender:"), this);
         descenderPromptLabel_ = new QLabel(tr("Descender:"), this);
   maxAdvanceWidthPromptLabel_ = new QLabel(tr("Max Advance Width:"), this);
  maxAdvanceHeightPromptLabel_ = new QLabel(tr("Max Advance Height:"), this);
             ulPosPromptLabel_ = new QLabel(tr("Underline Position:"), this);
       ulThicknessPromptLabel_ = new QLabel(tr("Underline Thickness:"), this);

            emSizeLabel_ = new QLabel(this);
              bboxLabel_ = new QLabel(this);
          ascenderLabel_ = new QLabel(this);
         descenderLabel_ = new QLabel(this);
   maxAdvanceWidthLabel_ = new QLabel(this);
  maxAdvanceHeightLabel_ = new QLabel(this);
             ulPosLabel_ = new QLabel(this);
       ulThicknessLabel_ = new QLabel(this);

  setLabelSelectable(          emSizeLabel_);
  setLabelSelectable(            bboxLabel_);
  setLabelSelectable(        ascenderLabel_);
  setLabelSelectable(       descenderLabel_);
  setLabelSelectable( maxAdvanceWidthLabel_);
  setLabelSelectable(maxAdvanceHeightLabel_);
  setLabelSelectable(           ulPosLabel_);
  setLabelSelectable(     ulThicknessLabel_);

  scalableOnlyLabels_.push_back(          emSizePromptLabel_);
  scalableOnlyLabels_.push_back(            bboxPromptLabel_);
  scalableOnlyLabels_.push_back(        ascenderPromptLabel_);
  scalableOnlyLabels_.push_back(       descenderPromptLabel_);
  scalableOnlyLabels_.push_back( maxAdvanceWidthPromptLabel_);
  scalableOnlyLabels_.push_back(maxAdvanceHeightPromptLabel_);
  scalableOnlyLabels_.push_back(           ulPosPromptLabel_);
  scalableOnlyLabels_.push_back(     ulThicknessPromptLabel_);
  scalableOnlyLabels_.push_back(          emSizeLabel_);
  scalableOnlyLabels_.push_back(            bboxLabel_);
  scalableOnlyLabels_.push_back(        ascenderLabel_);
  scalableOnlyLabels_.push_back(       descenderLabel_);
  scalableOnlyLabels_.push_back( maxAdvanceWidthLabel_);
  scalableOnlyLabels_.push_back(maxAdvanceHeightLabel_);
  scalableOnlyLabels_.push_back(           ulPosLabel_);
  scalableOnlyLabels_.push_back(     ulThicknessLabel_);

        basicGroupBox_ = new QGroupBox(tr("Basic"), this);
  typeEntriesGroupBox_ = new QGroupBox(tr("Type Entries"), this);
      charMapGroupBox_ = new QGroupBox(tr("CharMaps"), this);
  fixedSizesGroupBox_  = new QGroupBox(tr("Fixed Sizes"), this);

  charMapsTable_ = new QTableView(this);
  fixedSizesTable_ = new QTableView(this);

  charMapInfoModel_ = new CharMapInfoModel(this);
  charMapsTable_->setModel(charMapInfoModel_);
  auto header = charMapsTable_->verticalHeader();
  // This will force the minimal size to be used
  header->setDefaultSectionSize(0);
  header->setSectionResizeMode(QHeaderView::Fixed);

  fixedSizeInfoModel_ = new FixedSizeInfoModel(this);
  fixedSizesTable_->setModel(fixedSizeInfoModel_);
  header = fixedSizesTable_->verticalHeader();
  header->setDefaultSectionSize(0);
  header->setSectionResizeMode(QHeaderView::Fixed);

  basicLayout_       = new QGridLayout;
  typeEntriesLayout_ = new QGridLayout;
  charMapLayout_     = new QHBoxLayout;
  fixedSizesLayout_  = new QHBoxLayout;

#define GL2CRow(l, w) gridLayout2ColAddWidget(l          ,      \
                                              w##PromptLabel_,  \
                                              w##Label_)
#define BasicRow(w) GL2CRow(basicLayout_, w)
#define FTERow(w) GL2CRow(typeEntriesLayout_, w)

  BasicRow(    numFaces);
  BasicRow(      family);
  BasicRow(       style);
  BasicRow(  postscript);
  BasicRow(     created);
  BasicRow(    modified);
  BasicRow(    revision);
  BasicRow(   copyright);
  BasicRow(   trademark);
  BasicRow(manufacturer);

  FTERow(      driverName);
  FTERow(            sfnt);
  FTERow(        fontType);
  FTERow(       direction);
  FTERow(      fixedWidth);
  FTERow(      glyphNames);
  FTERow(          emSize);
  FTERow(            bbox);
  FTERow(        ascender);
  FTERow(       descender);
  FTERow( maxAdvanceWidth);
  FTERow(maxAdvanceHeight);
  FTERow(           ulPos);
  FTERow(     ulThickness);

  charMapLayout_->addWidget(charMapsTable_);
  fixedSizesLayout_->addWidget(fixedSizesTable_);

        basicGroupBox_ ->setLayout(basicLayout_      );
  typeEntriesGroupBox_ ->setLayout(typeEntriesLayout_);
      charMapGroupBox_ ->setLayout(charMapLayout_    );
  fixedSizesGroupBox_  ->setLayout(fixedSizesLayout_ );

  leftLayout_ = new QVBoxLayout;
  rightLayout_ = new QVBoxLayout;
  mainLayout_ = new QHBoxLayout;

  leftLayout_->addWidget(basicGroupBox_);
  leftLayout_->addWidget(typeEntriesGroupBox_);
  leftLayout_->addSpacerItem(new QSpacerItem(0, 0, 
                                             QSizePolicy::Preferred, 
                                             QSizePolicy::Expanding));

  rightLayout_->addWidget(charMapGroupBox_);
  rightLayout_->addWidget(fixedSizesGroupBox_);

  mainLayout_->addLayout(leftLayout_);
  mainLayout_->addLayout(rightLayout_);
  setLayout(mainLayout_);
}


SFNTInfoTab::SFNTInfoTab(QWidget* parent,
                         Engine* engine)
: QWidget(parent), engine_(engine)
{
  createLayout();
}


void
SFNTInfoTab::reloadFont()
{
  if (engine_->currentFontSFNTNames() != sfntNamesModel_->storage())
  {
    sfntNamesModel_->beginModelUpdate();
    sfntNamesModel_->storage() = engine_->currentFontSFNTNames();
    sfntNamesModel_->endModelUpdate();
  }
}


void
SFNTInfoTab::createLayout()
{
  sfntNamesGroupBox_ = new QGroupBox(tr("SFNT Name Table"), this);
  sfntTablesGroupBox_ = new QGroupBox(tr("SFNT Tables"), this);

  sfntNamesTable_ = new QTableView(this);
  sfntTablesTable_ = new QTableView(this);

  sfntNamesModel_ = new SFNTNameModel(this);
  sfntNamesTable_->setModel(sfntNamesModel_);
  auto header = sfntNamesTable_->verticalHeader();
  // This will force the minimal size to be used
  header->setDefaultSectionSize(0);
  header->setSectionResizeMode(QHeaderView::Fixed);
  sfntNamesTable_->horizontalHeader()->setStretchLastSection(true);

  header = sfntTablesTable_->verticalHeader();
  // This will force the minimal size to be used
  header->setDefaultSectionSize(0);
  header->setSectionResizeMode(QHeaderView::Fixed);

  sfntNamesLayout_ = new QHBoxLayout;
  sfntTablesLayout_ = new QHBoxLayout;

  sfntNamesLayout_->addWidget(sfntNamesTable_);
  sfntTablesLayout_->addWidget(sfntTablesTable_);

  sfntNamesGroupBox_->setLayout(sfntNamesLayout_);
  sfntTablesGroupBox_->setLayout(sfntTablesLayout_);

  mainLayout_ = new QHBoxLayout;

  mainLayout_->addWidget(sfntNamesGroupBox_);
  mainLayout_->addWidget(sfntTablesGroupBox_);

  setLayout(mainLayout_);
}


PostScriptInfoTab::PostScriptInfoTab(QWidget* parent,
                                     Engine* engine)
: QWidget(parent), engine_(engine)
{
}


void
PostScriptInfoTab::reloadFont()
{
}


MMGXInfoTab::MMGXInfoTab(QWidget* parent,
                         Engine* engine)
: QWidget(parent), engine_(engine)
{
}


void
MMGXInfoTab::reloadFont()
{
}


// end of info.cpp
