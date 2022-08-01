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
  glyphNameLabel_->setText(std::move(glyphName));

  auto rect = ctxt.basePosition.translated(-(ctxt.advance.x >> 6),
                                           -(ctxt.advance.y >> 6));
  bitmapWidget_->updateImage(ctxt.image, rect);
}


void
GlyphDetails::createLayout()
{
  glyphIndexPromptLabel_ = new QLabel(tr("Grid Index:"), this);
  charCodePromptLabel_ = new QLabel(tr("Char Code:"), this);
  glyphNamePromptLabel_ = new QLabel(tr("Glyph Name:"), this);

  glyphIndexLabel_ = new QLabel(this);
  charCodeLabel_ = new QLabel(this);
  glyphNameLabel_ = new QLabel(this);

  bitmapWidget_ = new GlyphBitmapWidget(this);

  setLabelSelectable(glyphIndexLabel_);
  setLabelSelectable(charCodeLabel_);
  setLabelSelectable(glyphNameLabel_);

  layout_ = new QGridLayout;
  layout_->addWidget(glyphIndexPromptLabel_, 0, 0);
  layout_->addWidget(charCodePromptLabel_, 1, 0);
  layout_->addWidget(glyphNamePromptLabel_, 2, 0);

  layout_->addWidget(glyphIndexLabel_, 0, 1);
  layout_->addWidget(charCodeLabel_, 1, 1);
  layout_->addWidget(glyphNameLabel_, 2, 1);
  layout_->addWidget(bitmapWidget_, 3, 0, 1, 2);

  layout_->setColumnStretch(1, 1);
  layout_->setRowStretch(3, 1);
  layout_->setSpacing(18);

  setLayout(layout_);
  setContentsMargins(12, 12, 12, 12);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}


void
GlyphDetails::createConnections()
{
}


// end of glyphdetails.cpp
