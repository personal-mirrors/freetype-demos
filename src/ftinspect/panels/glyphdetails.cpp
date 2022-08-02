// glyphdetails.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "glyphdetails.hpp"

#include "../engine/stringrenderer.hpp"
#include "../rendering/glyphcontinuous.hpp"
#include "../uihelper.hpp"
#include "../engine/engine.hpp"


GlyphDetails::GlyphDetails(QWidget* parent,
                           Engine* engine)
: QWidget(parent),
  engine_(engine)
{
  createLayout();
  createConnections();
}


GlyphDetails::~GlyphDetails()
{
}


void
GlyphDetails::updateGlyph(GlyphCacheEntry& ctxt, int charMapIndex)
{
  auto& cmaps = engine_->currentFontCharMaps();

  glyphIndexLabel_->setText(QString::number(ctxt.glyphIndex));
  if (charMapIndex < 0 || charMapIndex >= static_cast<int>(cmaps.size()))
  {
    charCodePromptLabel_->setVisible(false);
    charCodeLabel_->setVisible(false);
  }
  else
  {
    charCodePromptLabel_->setVisible(true);
    charCodeLabel_->setVisible(true);
    charCodeLabel_->setText(
        cmaps[charMapIndex].stringifyIndexShort(ctxt.charCode));
  }

  auto glyphName = engine_->glyphName(ctxt.glyphIndex);
  if (glyphName.isEmpty())
    glyphName = "(none)";
  glyphNameLabel_->setText(glyphName);

  auto rect = ctxt.basePosition.translated(-(ctxt.penPos.x()),
                                           -(ctxt.penPos.y()));
  bitmapWidget_->updateImage(ctxt.image, rect);

  // load glyphs in all units
  dpi_ = engine_->dpi();
  engine_->reloadFont();

  engine_->loadGlyphIntoSlotWithoutCache(ctxt.glyphIndex, true);
  fontUnitMetrics_ = engine_->currentFaceSlot()->metrics;
  engine_->loadGlyphIntoSlotWithoutCache(ctxt.glyphIndex, false);
  pixelMetrics_ = engine_->currentFaceSlot()->metrics;

  changeUnit(unitButtonGroup_->checkedId());

  inkSizeLabel_->setText(QString("(%1, %2) px")
                                 .arg(rect.width())
                                 .arg(rect.height()));
  bitmapOffsetLabel_->setText(QString("(%1, %2) px")
                                    .arg(rect.x())
                                    .arg(rect.y()));
}


void
GlyphDetails::createLayout()
{
  unitButtonGroup_ = new QButtonGroup(this);
  fontUnitButton_ = new QRadioButton(tr("Font Unit"), this);
  pointButton_ = new QRadioButton(tr("Point"), this);
  pixelButton_ = new QRadioButton(tr("Pixel"), this);
  unitButtonGroup_->addButton(fontUnitButton_, DU_FontUnit);
  unitButtonGroup_->addButton(pointButton_, DU_Point);
  unitButtonGroup_->addButton(pixelButton_, DU_Pixel);
  fontUnitButton_->setChecked(true);

  glyphIndexPromptLabel_ = new QLabel(tr("Grid Index:"), this);
  charCodePromptLabel_ = new QLabel(tr("Char Code:"), this);
  glyphNamePromptLabel_ = new QLabel(tr("Glyph Name:"), this);

  bboxSizePromptLabel_ = new QLabel(tr("Bounding Box Size:"), this);
  horiBearingPromptLabel_ = new QLabel(tr("Hori. Bearing:"), this);
  horiAdvancePromptLabel_ = new QLabel(tr("Hori. Advance:"), this);
  vertBearingPromptLabel_ = new QLabel(tr("Vert. Bearing:"), this);
  vertAdvancePromptLabel_ = new QLabel(tr("Vert. Advance:"), this);

  inkSizePromptLabel_ = new QLabel(tr("Ink Size:"), this);
  bitmapOffsetPromptLabel_ = new QLabel(tr("Bitmap Offset:"), this);

  glyphIndexLabel_ = new QLabel(this);
  charCodeLabel_ = new QLabel(this);
  glyphNameLabel_ = new QLabel(this);

  bboxSizeLabel_ = new QLabel(this);
  horiBearingLabel_ = new QLabel(this);
  horiAdvanceLabel_ = new QLabel(this);
  vertBearingLabel_ = new QLabel(this);
  vertAdvanceLabel_ = new QLabel(this);

  inkSizeLabel_ = new QLabel(this);
  bitmapOffsetLabel_ = new QLabel(this);

  bitmapWidget_ = new GlyphBitmapWidget(this);

  setLabelSelectable(glyphIndexLabel_);
  setLabelSelectable(charCodeLabel_);
  setLabelSelectable(glyphNameLabel_);
  setLabelSelectable(bboxSizeLabel_);
  setLabelSelectable(horiBearingLabel_);
  setLabelSelectable(horiAdvanceLabel_);
  setLabelSelectable(vertBearingLabel_);
  setLabelSelectable(vertAdvanceLabel_);
  setLabelSelectable(inkSizeLabel_);
  setLabelSelectable(bitmapOffsetLabel_);

  unitLayout_ = new QHBoxLayout;
  unitLayout_->addWidget(fontUnitButton_);
  unitLayout_->addWidget(pointButton_);
  unitLayout_->addWidget(pixelButton_);

  layout_ = new QGridLayout;
  layout_->addLayout(unitLayout_, 0, 0, 1, 2);
  layout_->addItem(new QSpacerItem(0, 18), 1, 0, 1, 2);

  layout_->addWidget(glyphIndexPromptLabel_, 2, 0);
  layout_->addWidget(charCodePromptLabel_,   3, 0);
  layout_->addWidget(glyphNamePromptLabel_,  4, 0);
  layout_->addItem(new QSpacerItem(0, 18), 5, 0, 1, 2);

  layout_->addWidget(bboxSizePromptLabel_,    6, 0);
  layout_->addWidget(horiBearingPromptLabel_, 7, 0);
  layout_->addWidget(horiAdvancePromptLabel_, 8, 0);
  layout_->addWidget(vertBearingPromptLabel_, 9, 0);
  layout_->addWidget(vertAdvancePromptLabel_, 10, 0);
  layout_->addItem(new QSpacerItem(0, 18), 11, 0, 1, 2);

  layout_->addWidget(inkSizePromptLabel_,      12, 0);
  layout_->addWidget(bitmapOffsetPromptLabel_, 13, 0);

  layout_->addWidget(glyphIndexLabel_, 2, 1);
  layout_->addWidget(charCodeLabel_,   3, 1);
  layout_->addWidget(glyphNameLabel_,  4, 1);

  layout_->addWidget(bboxSizeLabel_,    6, 1);
  layout_->addWidget(horiBearingLabel_, 7, 1);
  layout_->addWidget(horiAdvanceLabel_, 8, 1);
  layout_->addWidget(vertBearingLabel_, 9, 1);
  layout_->addWidget(vertAdvanceLabel_, 10, 1);

  layout_->addWidget(inkSizeLabel_,      12, 1);
  layout_->addWidget(bitmapOffsetLabel_, 13, 1);
  layout_->addItem(new QSpacerItem(0, 18), 14, 0, 1, 2);

  layout_->addWidget(bitmapWidget_, 15, 0, 1, 2);

  layout_->setColumnStretch(1, 1);
  layout_->setRowStretch(15, 1);

  setLayout(layout_);
  setContentsMargins(12, 12, 12, 12);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}


void
GlyphDetails::createConnections()
{
  connect(unitButtonGroup_, &QButtonGroup::idClicked,
          this, &GlyphDetails::changeUnit);
}


void
GlyphDetails::changeUnit(int unitId)
{
  QString unitSuffix;
  double bboxW = -1, bboxH = -1;
  double horiBearingX = -1, horiBearingY = -1;
  double horiAdvance = -1;
  double vertBearingX = -1, vertBearingY = -1;
  double vertAdvance = -1;
  switch (static_cast<DisplayUnit>(unitId))
  {
  case DU_FontUnit:
    unitSuffix = "";
    bboxW = fontUnitMetrics_.width;
    bboxH = fontUnitMetrics_.height;
    horiBearingX = fontUnitMetrics_.horiBearingX;
    horiBearingY = fontUnitMetrics_.horiBearingY;
    horiAdvance = fontUnitMetrics_.horiAdvance;
    vertBearingX = fontUnitMetrics_.vertBearingX;
    vertBearingY = fontUnitMetrics_.vertBearingY;
    vertAdvance = fontUnitMetrics_.vertAdvance;
    break;

  case DU_Point:
    unitSuffix = " pt";
    // 1.125 = 72 / 64
    bboxW = pixelMetrics_.width * 1.125 / dpi_;
    bboxH = pixelMetrics_.height * 1.125 / dpi_;
    horiBearingX = pixelMetrics_.horiBearingX * 1.125 / dpi_;
    horiBearingY = pixelMetrics_.horiBearingY * 1.125 / dpi_;
    horiAdvance = pixelMetrics_.horiAdvance * 1.125 / dpi_;
    vertBearingX = pixelMetrics_.vertBearingX * 1.125 / dpi_;
    vertBearingY = pixelMetrics_.vertBearingY * 1.125 / dpi_;
    vertAdvance = pixelMetrics_.vertAdvance * 1.125 / dpi_;
    break;
    
  case DU_Pixel:
    unitSuffix = " px";
    bboxW = pixelMetrics_.width/ 64.0;
    bboxH = pixelMetrics_.height/ 64.0;
    horiBearingX = pixelMetrics_.horiBearingX/ 64.0;
    horiBearingY = pixelMetrics_.horiBearingY/ 64.0;
    horiAdvance = pixelMetrics_.horiAdvance/ 64.0;
    vertBearingX = pixelMetrics_.vertBearingX/ 64.0;
    vertBearingY = pixelMetrics_.vertBearingY/ 64.0;
    vertAdvance = pixelMetrics_.vertAdvance/ 64.0;
    break;
  }

  auto tmpl = QString("%1") + unitSuffix;
  auto tmplPair = QString("(%1, %2)") + unitSuffix;
  bboxSizeLabel_->setText(tmplPair.arg(bboxW).arg(bboxH));
  horiBearingLabel_->setText(tmplPair.arg(horiBearingX).arg(horiBearingY));
  horiAdvanceLabel_->setText(tmpl.arg(horiAdvance));
  vertBearingLabel_->setText(tmplPair.arg(vertBearingX).arg(vertBearingY));
  vertAdvanceLabel_->setText(tmpl.arg(vertAdvance));
}


// end of glyphdetails.cpp
