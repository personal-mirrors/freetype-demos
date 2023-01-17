// info.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "info.hpp"
#include "../engine/engine.hpp"
#include "../uihelper.hpp"

#include <cstring>

#include <QHeaderView>
#include <QStringList>


#define GL2CRow(l, w) gridLayout2ColAddWidget(l, \
                                              w##PromptLabel_, \
                                              w##Label_)


InfoTab::InfoTab(QWidget* parent,
                 Engine* engine)
: QWidget(parent),
  engine_(engine)
{
  createLayout();
  createConnections();
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
  compositeGlyphsTab_ = new CompositeGlyphsTab(this, engine_);

  tab_ = new QTabWidget(this);
  tab_->addTab(generalTab_, tr("General"));
  tab_->addTab(sfntTab_, tr("SFNT"));
  tab_->addTab(postScriptTab_, tr("PostScript"));
  tab_->addTab(mmgxTab_, tr("MM/GX"));
  tab_->addTab(compositeGlyphsTab_, tr("Composite Glyphs"));

  tabs_.append(generalTab_);
  tabs_.append(sfntTab_);
  tabs_.append(postScriptTab_);
  tabs_.append(mmgxTab_);
  tabs_.append(compositeGlyphsTab_);

  layout_ = new QHBoxLayout;
  layout_->addWidget(tab_);

  setLayout(layout_);
}


void
InfoTab::createConnections()
{
  connect(compositeGlyphsTab_, &CompositeGlyphsTab::switchToSingular,
          this, &InfoTab::switchToSingular);
}


GeneralInfoTab::GeneralInfoTab(QWidget* parent,
                               Engine* engine)
: QWidget(parent),
  engine_(engine)
{
  createLayout();
}


void
GeneralInfoTab::reloadFont()
{
  auto basicInfo = FontBasicInfo::get(engine_);
  // Don't update if unnecessary.
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
      directionText = "horizontal, vertical";
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
  numFacesPromptLabel_ = new QLabel(tr("Num of Faces:"), this);
  familyPromptLabel_ = new QLabel(tr("Family Name:"), this);
  stylePromptLabel_ = new QLabel(tr("Style Name:"), this);
  postscriptPromptLabel_ = new QLabel(tr("PostScript Name:"), this);
  createdPromptLabel_ = new QLabel(tr("Created at:"), this);
  modifiedPromptLabel_ = new QLabel(tr("Modified at:"), this);
  revisionPromptLabel_ = new QLabel(tr("Font Revision:"), this);
  copyrightPromptLabel_ = new QLabel(tr("Copyright:"), this);
  trademarkPromptLabel_ = new QLabel(tr("Trademark:"), this);
  manufacturerPromptLabel_ = new QLabel(tr("Manufacturer:"), this);

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

  setLabelSelectable(numFacesLabel_);
  setLabelSelectable(familyLabel_);
  setLabelSelectable(styleLabel_);
  setLabelSelectable(postscriptLabel_);
  setLabelSelectable(createdLabel_);
  setLabelSelectable(modifiedLabel_);
  setLabelSelectable(revisionLabel_);
  setLabelSelectable(copyrightLabel_);
  setLabelSelectable(trademarkLabel_);
  setLabelSelectable(manufacturerLabel_);

  copyrightLabel_->setWordWrap(true);
  trademarkLabel_->setWordWrap(true);
  manufacturerLabel_->setWordWrap(true);

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
  setLabelSelectable(sfntLabel_);
  setLabelSelectable(fontTypeLabel_);
  setLabelSelectable(directionLabel_);
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

  setLabelSelectable(emSizeLabel_);
  setLabelSelectable(bboxLabel_);
  setLabelSelectable(ascenderLabel_);
  setLabelSelectable(descenderLabel_);
  setLabelSelectable(maxAdvanceWidthLabel_);
  setLabelSelectable(maxAdvanceHeightLabel_);
  setLabelSelectable(ulPosLabel_);
  setLabelSelectable(ulThicknessLabel_);

  scalableOnlyLabels_.push_back(emSizePromptLabel_);
  scalableOnlyLabels_.push_back(bboxPromptLabel_);
  scalableOnlyLabels_.push_back(ascenderPromptLabel_);
  scalableOnlyLabels_.push_back(descenderPromptLabel_);
  scalableOnlyLabels_.push_back(maxAdvanceWidthPromptLabel_);
  scalableOnlyLabels_.push_back(maxAdvanceHeightPromptLabel_);
  scalableOnlyLabels_.push_back(ulPosPromptLabel_);
  scalableOnlyLabels_.push_back(ulThicknessPromptLabel_);
  scalableOnlyLabels_.push_back(emSizeLabel_);
  scalableOnlyLabels_.push_back(bboxLabel_);
  scalableOnlyLabels_.push_back(ascenderLabel_);
  scalableOnlyLabels_.push_back(descenderLabel_);
  scalableOnlyLabels_.push_back(maxAdvanceWidthLabel_);
  scalableOnlyLabels_.push_back(maxAdvanceHeightLabel_);
  scalableOnlyLabels_.push_back(ulPosLabel_);
  scalableOnlyLabels_.push_back(ulThicknessLabel_);

  basicGroupBox_ = new QGroupBox(tr("Basic"), this);
  typeEntriesGroupBox_ = new QGroupBox(tr("Type Entries"), this);
  charMapGroupBox_ = new QGroupBox(tr("CharMaps"), this);
  fixedSizesGroupBox_ = new QGroupBox(tr("Fixed Sizes"), this);

  charMapsTable_ = new QTableView(this);
  fixedSizesTable_ = new QTableView(this);

  charMapInfoModel_ = new CharMapInfoModel(this);
  charMapsTable_->setModel(charMapInfoModel_);
  auto header = charMapsTable_->verticalHeader();
  // This forces the minimum size to be used.
  header->setDefaultSectionSize(0);
  header->setSectionResizeMode(QHeaderView::Fixed);

  fixedSizeInfoModel_ = new FixedSizeInfoModel(this);
  fixedSizesTable_->setModel(fixedSizeInfoModel_);
  header = fixedSizesTable_->verticalHeader();
  header->setDefaultSectionSize(0);
  header->setSectionResizeMode(QHeaderView::Fixed);

  leftWidget_ = new QWidget(this);
  leftScrollArea_ = new UnboundScrollArea(this);
  leftScrollArea_->setWidgetResizable(true);
  leftScrollArea_->setWidget(leftWidget_);
  leftScrollArea_->setStyleSheet(
                     "QScrollArea {background-color:transparent;}");
  leftWidget_->setStyleSheet("background-color:transparent;");

  basicLayout_ = new QGridLayout;
  typeEntriesLayout_ = new QGridLayout;
  charMapLayout_ = new QHBoxLayout;
  fixedSizesLayout_ = new QHBoxLayout;

#define BasicRow(w) GL2CRow(basicLayout_, w)
#define FTERow(w) GL2CRow(typeEntriesLayout_, w)

  BasicRow(numFaces);
  BasicRow(family);
  BasicRow(style);
  BasicRow(postscript);
  BasicRow(created);
  BasicRow(modified);
  BasicRow(revision);
  BasicRow(copyright);
  BasicRow(trademark);
  BasicRow(manufacturer);

  FTERow(driverName);
  FTERow(sfnt);
  FTERow(fontType);
  FTERow(direction);
  FTERow(fixedWidth);
  FTERow(glyphNames);
  FTERow(emSize);
  FTERow(bbox);
  FTERow(ascender);
  FTERow(descender);
  FTERow(maxAdvanceWidth);
  FTERow(maxAdvanceHeight);
  FTERow(ulPos);
  FTERow(ulThickness);

  basicLayout_->setColumnStretch(1, 1);
  typeEntriesLayout_->setColumnStretch(1, 1);

  charMapLayout_->addWidget(charMapsTable_);
  fixedSizesLayout_->addWidget(fixedSizesTable_);

  basicGroupBox_ ->setLayout(basicLayout_);
  typeEntriesGroupBox_ ->setLayout(typeEntriesLayout_);
  charMapGroupBox_ ->setLayout(charMapLayout_);
  fixedSizesGroupBox_ ->setLayout(fixedSizesLayout_);

  leftLayout_ = new QVBoxLayout;
  rightLayout_ = new QVBoxLayout;
  mainLayout_ = new QHBoxLayout;

  leftLayout_->addWidget(basicGroupBox_);
  leftLayout_->addWidget(typeEntriesGroupBox_);
  leftLayout_->addSpacerItem(new QSpacerItem(0, 0,
                                             QSizePolicy::Preferred,
                                             QSizePolicy::Expanding));

  leftWidget_->setLayout(leftLayout_);

  rightLayout_->addWidget(charMapGroupBox_);
  rightLayout_->addWidget(fixedSizesGroupBox_);

  mainLayout_->addWidget(leftScrollArea_);
  mainLayout_->addLayout(rightLayout_);
  setLayout(mainLayout_);
}


StringViewDialog::StringViewDialog(QWidget* parent)
: QDialog(parent)
{
  createLayout();
}


void
StringViewDialog::updateString(QByteArray const& rawArray,
                               QString const& str)
{
  textEdit_->setText(str);
  hexTextEdit_->setText(rawArray.toHex());
}


void
StringViewDialog::createLayout()
{
  textEdit_ = new QTextEdit(this);
  hexTextEdit_ = new QTextEdit(this);

  textEdit_->setLineWrapMode(QTextEdit::WidgetWidth);
  hexTextEdit_->setLineWrapMode(QTextEdit::WidgetWidth);

  textLabel_ = new QLabel(tr("Text"), this);
  hexTextLabel_ = new QLabel(tr("Raw Bytes"), this);

  layout_ = new QVBoxLayout;

  layout_->addWidget(textLabel_);
  layout_->addWidget(textEdit_);
  layout_->addWidget(hexTextLabel_);
  layout_->addWidget(hexTextEdit_);

  resize(600, 400);

  setLayout(layout_);
}


SFNTInfoTab::SFNTInfoTab(QWidget* parent,
                         Engine* engine)
: QWidget(parent),
  engine_(engine)
{
  createLayout();
  createConnections();
}


void
SFNTInfoTab::reloadFont()
{
  engine_->reloadFont();
  auto face = engine_->currentFallbackFtFace();
  setEnabled(face && FT_IS_SFNT(face));

  if (engine_->currentFontSFNTNames() != sfntNamesModel_->storage())
  {
    sfntNamesModel_->beginModelUpdate();
    sfntNamesModel_->storage() = engine_->currentFontSFNTNames();
    sfntNamesModel_->endModelUpdate();
  }

  if (engine_->currentFontSFNTTableInfo() != sfntTablesModel_->storage())
  {
    sfntTablesModel_->beginModelUpdate();
    sfntTablesModel_->storage() = engine_->currentFontSFNTTableInfo();
    sfntTablesModel_->endModelUpdate();
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
  // This forces the minimum size to be used.
  header->setDefaultSectionSize(0);
  header->setSectionResizeMode(QHeaderView::Fixed);
  sfntNamesTable_->horizontalHeader()->setStretchLastSection(true);

  sfntTablesModel_ = new SFNTTableInfoModel(this);
  sfntTablesTable_->setModel(sfntTablesModel_);
  header = sfntTablesTable_->verticalHeader();
  // This forces the minimum size to be used.
  header->setDefaultSectionSize(0);
  header->setSectionResizeMode(QHeaderView::Fixed);
  sfntTablesTable_->horizontalHeader()->setStretchLastSection(true);

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

  stringViewDialog_ = new StringViewDialog(this);
}


void
SFNTInfoTab::createConnections()
{
  connect(sfntNamesTable_, &QTableView::doubleClicked,
          this, &SFNTInfoTab::nameTableDoubleClicked);
}


void
SFNTInfoTab::nameTableDoubleClicked(QModelIndex const& index)
{
  if (index.column() != SFNTNameModel::SNM_Content)
    return;
  auto& storage = sfntNamesModel_->storage();
  if (index.row() < 0 || static_cast<size_t>(index.row()) > storage.size())
    return;

  auto& obj = storage[index.row()];
  stringViewDialog_->updateString(obj.strBuf, obj.str);
  stringViewDialog_->exec();
}


PostScriptInfoTab::PostScriptInfoTab(QWidget* parent,
                                     Engine* engine)
: QWidget(parent),
  engine_(engine)
{
  std::memset(&oldFontPrivate_, 0, sizeof(PS_PrivateRec));
  createLayout();
}


template<class T>
QString genArrayString(T* arr,
                       size_t size)
{
  // TODO: optimize
  QString result = "[";
  for (size_t i = 0; i < size; i++)
  {
    result += QString::number(arr[i]);
    if (i < size - 1)
      result += ", ";
  }
  return result + "]";
}


// We don't have C++20, so...
template <class T, std::ptrdiff_t N>
constexpr std::ptrdiff_t
arraySize(const T (&)[N]) noexcept
{
  return N;
}


void
PostScriptInfoTab::reloadFont()
{
  PS_FontInfoRec fontInfo;
  auto hasInfo = engine_->currentFontPSInfo(fontInfo);
  infoGroupBox_->setEnabled(hasInfo);
  if (hasInfo)
  {
    versionLabel_->setText(QString::fromUtf8(fontInfo.version));
    noticeLabel_->setText(QString::fromUtf8(fontInfo.notice));
    fullNameLabel_->setText(QString::fromUtf8(fontInfo.full_name));
    familyNameLabel_->setText(QString::fromUtf8(fontInfo.family_name));
    weightLabel_->setText(QString::fromUtf8(fontInfo.weight));
    italicAngleLabel_->setText(QString::number(fontInfo.italic_angle));
    fixedPitchLabel_->setText(fontInfo.is_fixed_pitch ? "yes" : "no");
    ulPosLabel_->setText(QString::number(fontInfo.underline_position));
    ulThicknessLabel_->setText(QString::number(fontInfo.underline_thickness));
  }
  else
  {
    versionLabel_->clear();
    noticeLabel_->clear();
    fullNameLabel_->clear();
    familyNameLabel_->clear();
    weightLabel_->clear();
    italicAngleLabel_->clear();
    fixedPitchLabel_->clear();
    ulPosLabel_->clear();
    ulThicknessLabel_->clear();
  }

  PS_PrivateRec fontPrivate;
  // Don't do zero-initialization since we need to zero out paddings.
  std::memset(&fontPrivate, 0, sizeof(PS_PrivateRec));
  hasInfo = engine_->currentFontPSPrivateInfo(fontPrivate);
  privateGroupBox_->setEnabled(hasInfo);
  if (hasInfo)
  {
    if (std::memcmp(&fontPrivate, &oldFontPrivate_, sizeof(PS_PrivateRec)))
    {
      std::memcpy(&oldFontPrivate_, &fontPrivate, sizeof(PS_PrivateRec));

      uniqueIDLabel_->setText(QString::number(fontPrivate.unique_id));
      blueShiftLabel_->setText(QString::number(fontPrivate.blue_shift));
      blueFuzzLabel_->setText(QString::number(fontPrivate.blue_fuzz));
      forceBoldLabel_->setText(fontPrivate.force_bold ? "true" : "false");
      languageGroupLabel_->setText(
                             QString::number(fontPrivate.language_group));
      passwordLabel_->setText(QString::number(fontPrivate.password));
      lenIVLabel_->setText(QString::number(fontPrivate.lenIV));
      roundStemUpLabel_->setText(fontPrivate.round_stem_up ? "true"
                                                           : "false");

      familyBluesLabel_->setText(
        genArrayString(fontPrivate.family_blues,
                       fontPrivate.num_family_blues));
      blueValuesLabel_->setText(
        genArrayString(fontPrivate.blue_values,
                       fontPrivate.num_blue_values));
      otherBluesLabel_->setText(
        genArrayString(fontPrivate.other_blues,
                       fontPrivate.num_other_blues));
      familyOtherBluesLabel_->setText(
        genArrayString(fontPrivate.family_other_blues,
                       fontPrivate.num_family_other_blues));
      stdWidthsLabel_->setText(
        genArrayString(fontPrivate.standard_width,
                       arraySize(fontPrivate.standard_width)));
      stdHeightsLabel_->setText(
        genArrayString(fontPrivate.standard_height,
                       arraySize(fontPrivate.standard_height)));
      snapWidthsLabel_->setText(
        genArrayString(fontPrivate.snap_widths,
                       fontPrivate.num_snap_widths));
      snapHeightsLabel_->setText(
        genArrayString(fontPrivate.snap_heights,
                       fontPrivate.num_snap_heights));
      minFeatureLabel_->setText(
        genArrayString(fontPrivate.min_feature,
                       arraySize(fontPrivate.min_feature)));

      blueScaleLabel_->setText(
        QString::number(fontPrivate.blue_scale / 65536.0 / 1000.0, 'f', 6));
      expansionFactorLabel_->setText(
        QString::number(fontPrivate.expansion_factor / 65536.0, 'f', 4));
    }
  }
  else
  {
    std::memset(&oldFontPrivate_, 0, sizeof(PS_PrivateRec));

    uniqueIDLabel_->clear();
    blueValuesLabel_->clear();
    otherBluesLabel_->clear();
    familyBluesLabel_->clear();
    familyOtherBluesLabel_->clear();
    blueScaleLabel_->clear();
    blueShiftLabel_->clear();
    blueFuzzLabel_->clear();
    stdWidthsLabel_->clear();
    stdHeightsLabel_->clear();
    snapWidthsLabel_->clear();
    snapHeightsLabel_->clear();
    forceBoldLabel_->clear();
    languageGroupLabel_->clear();
    passwordLabel_->clear();
    lenIVLabel_->clear();
    minFeatureLabel_->clear();
    roundStemUpLabel_->clear();
    expansionFactorLabel_->clear();
  }
}


void
PostScriptInfoTab::createLayout()
{
  versionPromptLabel_ = new QLabel(tr("/version:"), this);
  noticePromptLabel_ = new QLabel(tr("/Notice:"), this);
  fullNamePromptLabel_ = new QLabel(tr("/FullName:"), this);
  familyNamePromptLabel_ = new QLabel(tr("/FamilyName:"), this);
  weightPromptLabel_ = new QLabel(tr("/Weight:"), this);
  italicAnglePromptLabel_ = new QLabel(tr("/ItaticAngle:"), this);
  fixedPitchPromptLabel_ = new QLabel(tr("/isFixedPitch:"), this);
  ulPosPromptLabel_ = new QLabel(tr("/UnderlinePosition:"), this);
  ulThicknessPromptLabel_ = new QLabel(tr("/UnderlineThickness:"), this);

  versionLabel_ = new QLabel(this);
  noticeLabel_ = new QLabel(this);
  fullNameLabel_ = new QLabel(this);
  familyNameLabel_ = new QLabel(this);
  weightLabel_ = new QLabel(this);
  italicAngleLabel_ = new QLabel(this);
  fixedPitchLabel_ = new QLabel(this);
  ulPosLabel_ = new QLabel(this);
  ulThicknessLabel_ = new QLabel(this);

  setLabelSelectable(versionLabel_);
  setLabelSelectable(noticeLabel_);
  setLabelSelectable(fullNameLabel_);
  setLabelSelectable(familyNameLabel_);
  setLabelSelectable(weightLabel_);
  setLabelSelectable(italicAngleLabel_);
  setLabelSelectable(fixedPitchLabel_);
  setLabelSelectable(ulPosLabel_);
  setLabelSelectable(ulThicknessLabel_);

  uniqueIDPromptLabel_ = new QLabel(tr("/UniqueID:"), this);
  blueValuesPromptLabel_ = new QLabel(tr("/BlueValues:"), this);
  otherBluesPromptLabel_ = new QLabel(tr("/OtherBlues:"), this);
  familyBluesPromptLabel_ = new QLabel(tr("/FamilyBlues:"), this);
  familyOtherBluesPromptLabel_ = new QLabel(tr("/FamilyOtherBlues:"), this);
  blueScalePromptLabel_ = new QLabel(tr("/BlueScale:"), this);
  blueShiftPromptLabel_ = new QLabel(tr("/BlueShift:"), this);
  blueFuzzPromptLabel_ = new QLabel(tr("/BlueFuzz:"), this);
  stdWidthsPromptLabel_ = new QLabel(tr("/StdHW:"), this);
  stdHeightsPromptLabel_ = new QLabel(tr("/StdVW:"), this);
  snapWidthsPromptLabel_ = new QLabel(tr("/StemSnapH:"), this);
  snapHeightsPromptLabel_ = new QLabel(tr("/StemSnapV:"), this);
  forceBoldPromptLabel_ = new QLabel(tr("/ForceBold:"), this);
  languageGroupPromptLabel_ = new QLabel(tr("/LanguageGroup:"), this);
  passwordPromptLabel_ = new QLabel(tr("/password:"), this);
  lenIVPromptLabel_ = new QLabel(tr("/lenIV:"), this);
  minFeaturePromptLabel_ = new QLabel(tr("/MinFeature:"), this);
  roundStemUpPromptLabel_ = new QLabel(tr("/RndStemUp:"), this);
  expansionFactorPromptLabel_ = new QLabel(tr("/ExpansionFactor:"), this);

  uniqueIDLabel_ = new QLabel(this);
  blueValuesLabel_ = new QLabel(this);
  otherBluesLabel_ = new QLabel(this);
  familyBluesLabel_ = new QLabel(this);
  familyOtherBluesLabel_ = new QLabel(this);
  blueScaleLabel_ = new QLabel(this);
  blueShiftLabel_ = new QLabel(this);
  blueFuzzLabel_ = new QLabel(this);
  stdWidthsLabel_ = new QLabel(this);
  stdHeightsLabel_ = new QLabel(this);
  snapWidthsLabel_ = new QLabel(this);
  snapHeightsLabel_ = new QLabel(this);
  forceBoldLabel_ = new QLabel(this);
  languageGroupLabel_ = new QLabel(this);
  passwordLabel_ = new QLabel(this);
  lenIVLabel_ = new QLabel(this);
  minFeatureLabel_ = new QLabel(this);
  roundStemUpLabel_ = new QLabel(this);
  expansionFactorLabel_ = new QLabel(this);

  setLabelSelectable(uniqueIDLabel_);
  setLabelSelectable(blueValuesLabel_);
  setLabelSelectable(otherBluesLabel_);
  setLabelSelectable(familyBluesLabel_);
  setLabelSelectable(familyOtherBluesLabel_);
  setLabelSelectable(blueScaleLabel_);
  setLabelSelectable(blueShiftLabel_);
  setLabelSelectable(blueFuzzLabel_);
  setLabelSelectable(stdWidthsLabel_);
  setLabelSelectable(stdHeightsLabel_);
  setLabelSelectable(snapWidthsLabel_);
  setLabelSelectable(snapHeightsLabel_);
  setLabelSelectable(forceBoldLabel_);
  setLabelSelectable(languageGroupLabel_);
  setLabelSelectable(passwordLabel_);
  setLabelSelectable(lenIVLabel_);
  setLabelSelectable(minFeatureLabel_);
  setLabelSelectable(roundStemUpLabel_);
  setLabelSelectable(expansionFactorLabel_);

  noticeLabel_->setWordWrap(true);
  familyBluesLabel_->setWordWrap(true);
  blueValuesLabel_->setWordWrap(true);
  otherBluesLabel_->setWordWrap(true);
  familyOtherBluesLabel_->setWordWrap(true);
  stdWidthsLabel_->setWordWrap(true);
  stdHeightsLabel_->setWordWrap(true);
  snapWidthsLabel_->setWordWrap(true);
  snapHeightsLabel_->setWordWrap(true);
  minFeatureLabel_->setWordWrap(true);

  infoGroupBox_ = new QGroupBox(tr("PostScript /FontInfo dictionary"),
                                this);
  privateGroupBox_ = new QGroupBox(tr("PostScript /Private dictionary"),
                                   this);

  infoWidget_ = new QWidget(this);
  privateWidget_ = new QWidget(this);

  infoScrollArea_ = new UnboundScrollArea(this);
  infoScrollArea_->setWidget(infoWidget_);
  infoScrollArea_->setWidgetResizable(true);
  infoScrollArea_->setStyleSheet(
                     "QScrollArea {background-color:transparent;}");
  infoWidget_->setStyleSheet("background-color:transparent;");
  infoWidget_->setContentsMargins(0, 0, 0, 0);

  privateScrollArea_ = new UnboundScrollArea(this);
  privateScrollArea_->setWidget(privateWidget_);
  privateScrollArea_->setWidgetResizable(true);
  privateScrollArea_->setStyleSheet(
                        "QScrollArea {background-color:transparent;}");
  privateWidget_->setStyleSheet("background-color:transparent;");
  privateWidget_->setContentsMargins(0, 0, 0, 0);

  infoLayout_ = new QGridLayout;
  privateLayout_ = new QGridLayout;
  infoGroupBoxLayout_ = new QHBoxLayout;
  privateGroupBoxLayout_ = new QHBoxLayout;

#define PSI2Row(w) GL2CRow(infoLayout_, w)
#define PSP2Row(w) GL2CRow(privateLayout_, w)

  PSI2Row(version);
  PSI2Row(notice);
  PSI2Row(fullName);
  PSI2Row(familyName);
  PSI2Row(weight);
  PSI2Row(italicAngle);
  PSI2Row(fixedPitch);
  PSI2Row(ulPos);
  PSI2Row(ulThickness);

  PSP2Row(uniqueID);
  PSP2Row(blueValues);
  PSP2Row(otherBlues);
  PSP2Row(familyBlues);
  PSP2Row(familyOtherBlues);
  PSP2Row(blueScale);
  PSP2Row(blueShift);
  PSP2Row(blueFuzz);
  PSP2Row(stdWidths);
  PSP2Row(stdHeights);
  PSP2Row(snapWidths);
  PSP2Row(snapHeights);
  PSP2Row(forceBold);
  PSP2Row(languageGroup);
  PSP2Row(password);
  PSP2Row(lenIV);
  PSP2Row(minFeature);
  PSP2Row(roundStemUp);
  PSP2Row(expansionFactor);

  infoLayout_->addItem(new QSpacerItem(0, 0,
                                       QSizePolicy::Preferred,
                                       QSizePolicy::Expanding),
                       infoLayout_->rowCount(), 0, 1, 2);
  privateLayout_->addItem(new QSpacerItem(0, 0,
                                          QSizePolicy::Preferred,
                                          QSizePolicy::Expanding),
                          privateLayout_->rowCount(), 0, 1, 2);

  infoLayout_->setColumnStretch(1, 1);
  privateLayout_->setColumnStretch(1, 1);

  infoWidget_->setLayout(infoLayout_);
  privateWidget_->setLayout(privateLayout_);
  infoGroupBox_->setSizePolicy(QSizePolicy::Ignored,
                               QSizePolicy::Expanding);
  privateGroupBox_->setSizePolicy(QSizePolicy::Ignored,
                                  QSizePolicy::Expanding);

  infoGroupBoxLayout_->addWidget(infoScrollArea_);
  privateGroupBoxLayout_->addWidget(privateScrollArea_);
  infoGroupBox_->setLayout(infoGroupBoxLayout_);
  privateGroupBox_->setLayout(privateGroupBoxLayout_);

  mainLayout_ = new QHBoxLayout;
  mainLayout_->addWidget(infoGroupBox_);
  mainLayout_->addWidget(privateGroupBox_);
  setLayout(mainLayout_);
}


MMGXInfoTab::MMGXInfoTab(QWidget* parent,
                         Engine* engine)
: QWidget(parent),
  engine_(engine)
{
  createLayout();
}


void
MMGXInfoTab::reloadFont()
{
  auto state = engine_->currentFontMMGXState();
  axesGroupBox_->setEnabled(state != MMGXState::NoMMGX);
  switch (state)
  {
  case MMGXState::NoMMGX:
    mmgxTypeLabel_->setText("No MM/GX");
    break;
  case MMGXState::MM:
    mmgxTypeLabel_->setText("Adobe Multiple Master");
    break;
  case MMGXState::GX_OVF:
    mmgxTypeLabel_->setText("TrueType GX or OpenType Variable Font");
    break;
  default:
    mmgxTypeLabel_->setText("Unknown");
  }

  if (engine_->currentFontMMGXAxes() != axesModel_->storage())
  {
    axesModel_->beginModelUpdate();
    axesModel_->storage() = engine_->currentFontMMGXAxes();
    axesModel_->endModelUpdate();
  }

  setEnabled(state != MMGXState::NoMMGX);
}


void
MMGXInfoTab::createLayout()
{
  mmgxTypePromptLabel_ = new QLabel(tr("MM/GX Type:"));
  mmgxTypeLabel_ = new QLabel(this);
  setLabelSelectable(mmgxTypeLabel_);

  axesTable_ = new QTableView(this);

  axesModel_ = new MMGXAxisInfoModel(this);
  axesTable_->setModel(axesModel_);
  auto header = axesTable_->verticalHeader();
  // This forces the minimum size to be used.
  header->setDefaultSectionSize(0);
  header->setSectionResizeMode(QHeaderView::Fixed);
  axesTable_->horizontalHeader()->setStretchLastSection(true);

  axesGroupBox_ = new QGroupBox("MM/GX Axes");

  axesLayout_ = new QHBoxLayout;
  axesLayout_->addWidget(axesTable_);

  axesGroupBox_->setLayout(axesLayout_);

  infoLayout_ = new QGridLayout;
#define MMGXI2Row(w) GL2CRow(infoLayout_, w)
  auto r = MMGXI2Row(mmgxType);

  infoLayout_->addItem(new QSpacerItem(0, 0,
                                       QSizePolicy::Expanding,
                                       QSizePolicy::Preferred),
                       r, 2);

  mainLayout_ = new QVBoxLayout;
  mainLayout_->addLayout(infoLayout_);
  mainLayout_->addWidget(axesGroupBox_, 1);

  setLayout(mainLayout_);
}


CompositeGlyphsTab::CompositeGlyphsTab(QWidget* parent,
                                       Engine* engine)
: QWidget(parent),
  engine_(engine)
{
  createLayout();
  createConnections();
}


void
CompositeGlyphsTab::reloadFont()
{
  if (engine_->fontFileManager().currentReloadDueToPeriodicUpdate())
    return;
  forceReloadFont();
}


void
CompositeGlyphsTab::createLayout()
{
  compositeGlyphCountPromptLabel_ = new QLabel(tr("Composite Glyphs Count:"));
  compositeGlyphCountLabel_ = new QLabel(this);
  forceRefreshButton_ = new QPushButton(tr("Force Refresh"), this);
  compositeTreeView_ = new QTreeView(this);

  compositeModel_ = new CompositeGlyphsInfoModel(this, engine_);
  compositeTreeView_->setModel(compositeModel_);

  forceRefreshButton_->setToolTip(tr(
    "Force refresh the tree view.\n"
    "Note that periodic reloading of fonts loaded from symbolic links won't\n"
    "trigger automatically refreshing, so you need to manually reload."));

  // Layouting
  countLayout_ = new QHBoxLayout;
  countLayout_->addWidget(compositeGlyphCountPromptLabel_);
  countLayout_->addWidget(compositeGlyphCountLabel_);
  countLayout_->addWidget(forceRefreshButton_);
  countLayout_->addStretch(1);

  mainLayout_ = new QVBoxLayout;
  mainLayout_->addLayout(countLayout_);
  mainLayout_->addWidget(compositeTreeView_);

  setLayout(mainLayout_);
}


void
CompositeGlyphsTab::createConnections()
{
  connect(forceRefreshButton_, &QPushButton::clicked,
          this, &CompositeGlyphsTab::forceReloadFont);
  connect(compositeTreeView_, &QTreeView::doubleClicked,
          this, &CompositeGlyphsTab::treeRowDoubleClicked);
}


void
CompositeGlyphsTab::forceReloadFont()
{
  engine_->loadDefaults(); // This reloads the font.
  auto face = engine_->currentFallbackFtFace();

  std::vector<CompositeGlyphInfo> list;
  CompositeGlyphInfo::get(engine_, list);
  if (list != compositeModel_->storage())
  {
    compositeModel_->beginModelUpdate();
    compositeModel_->storage() = std::move(list);
    compositeModel_->endModelUpdate();
  }

  if (!face || !FT_IS_SFNT(face))
  {
    compositeGlyphCountPromptLabel_->setVisible(false);
    compositeGlyphCountLabel_->setText(tr("Not an SFNT font."));
  }
  else if (compositeModel_->storage().empty())
  {
    compositeGlyphCountPromptLabel_->setVisible(false);
    compositeGlyphCountLabel_->setText(
      tr("No composite glyphs in the 'glyf' table."));
  }
  else
  {
    compositeGlyphCountPromptLabel_->setVisible(true);
    compositeGlyphCountLabel_->setText(
      QString::number(compositeModel_->storage().size()));
  }
}


void
CompositeGlyphsTab::treeRowDoubleClicked(const QModelIndex& idx)
{
  auto gidx = compositeModel_->glyphIndexFromIndex(idx);
  if (gidx < 0)
    return;
  emit switchToSingular(gidx);
}


// end of info.cpp
