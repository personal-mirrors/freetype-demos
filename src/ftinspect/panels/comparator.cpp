// comparator.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "comparator.hpp"

#include <QScrollBar>

namespace
{
// No C++17 :-(
extern const char* ComparatorDefaultText;
}

ComperatorTab::ComperatorTab(QWidget* parent, Engine* engine)
: QWidget(parent),
  engine_(engine)
{
  createLayout();
  createConnections();
  setupCanvases();
}


ComperatorTab::~ComperatorTab()
{
}


void
ComperatorTab::repaintGlyph()
{
  sizeSelector_->applyToEngine(engine_);

  int i = 0;
  for (auto canvas : canvas_)
  {
    syncSettings(i);
    // No cache here, sry, because when switching between columns, the hinting
    // mode or enabling of embedded bitmaps may differ
    canvas->stringRenderer().reloadGlyphs();
    canvas->purgeCache();
    canvas->repaint();
    i++;
  }
}


void
ComperatorTab::reloadFont()
{
  {
    QSignalBlocker blocker(sizeSelector_);
    sizeSelector_->reloadFromFont(engine_); 
  }
  charMapSelector_->repopulate();
  for (auto panel : settingPanels_)
    panel->onFontChanged();

  for (auto canvas : canvas_)
    canvas->stringRenderer().reloadAll();
  repaintGlyph();
}


bool
ComperatorTab::eventFilter(QObject* watched,
                           QEvent* event)
{
  auto ptr = qobject_cast<GlyphContinuous*>(watched);
  if (ptr && event->type() == QEvent::Resize)
    return true;
  return QWidget::eventFilter(watched, event);
}


void
ComperatorTab::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  forceEqualWidths();
  repaintGlyph();
}


void
ComperatorTab::createLayout()
{
  sizeSelector_ = new FontSizeSelector(this);
  charMapLabel_ = new QLabel(tr("Char Map:"), this);
  charMapSelector_ = new CharMapComboBox(this, engine_, false);

  sourceTextEdit_ = new QPlainTextEdit(QString(ComparatorDefaultText), this);

  for (int i = 0; i < ColumnWidth; ++i)
  {
    auto frame = new QFrame(this);
    auto canvas = new GlyphContinuous(frame, engine_);
    auto settingPanel = new SettingPanel(this, engine_, true);
    
    canvas->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    settingPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    canvas_.emplace_back(canvas);
    settingPanels_.emplace_back(settingPanel);
    frames_.emplace_back(frame);

    auto frameLayout = new QHBoxLayout;
    frameLayout->addWidget(canvas);
    frame->setLayout(frameLayout);
    frame->setContentsMargins(2, 2, 2, 2);
    frameLayout->setContentsMargins(2, 2, 2, 2);
    frame->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  }

  sourceWidget_ = new QWidget(this);
  sourceWidget_->setMaximumWidth(350);

  layout_ = new QGridLayout;

  charMapLayout_ = new QHBoxLayout;
  charMapLayout_->addWidget(charMapLabel_);
  charMapLayout_->addWidget(charMapSelector_);

  sourceLayout_ = new QVBoxLayout;
  sourceLayout_->addWidget(sizeSelector_);
  sourceLayout_->addLayout(charMapLayout_);
  sourceLayout_->addWidget(sourceTextEdit_, 1);
  sourceWidget_->setLayout(sourceLayout_);

  layout_->addWidget(sourceWidget_, 0, 0, 2, 1);
  for (int i = 0; static_cast<unsigned>(i) < frames_.size(); ++i)
    layout_->addWidget(frames_[i], 0, i + 1);
  for (int i = 0; static_cast<unsigned>(i) < settingPanels_.size(); ++i)
    layout_->addWidget(settingPanels_[i], 1, i + 1);

  layout_->setRowStretch(0, 3);
  layout_->setRowStretch(1, 2);

  setLayout(layout_);

  forceEqualWidths();
}


void
ComperatorTab::createConnections()
{
  connect(sizeSelector_, &FontSizeSelector::valueChanged,
          this, &ComperatorTab::reloadGlyphsAndRepaint);
  connect(sourceTextEdit_, &QPlainTextEdit::textChanged,
          this, &ComperatorTab::sourceTextChanged);
  connect(charMapSelector_,
          QOverload<int>::of(&CharMapComboBox::currentIndexChanged),
          this, &ComperatorTab::reloadStringAndRepaint);

  for (auto panel : settingPanels_)
  {
    // We're treating the two events identically, because we need to do a
    // complete flush anyway
    connect(panel, &SettingPanel::repaintNeeded,
            this, &ComperatorTab::repaintGlyph);
    connect(panel, &SettingPanel::fontReloadNeeded,
            this, &ComperatorTab::repaintGlyph);
  }
}


void
ComperatorTab::setupCanvases()
{
  for (auto canvas : canvas_)
  {
    canvas->setMode(GlyphContinuous::M_Normal);
    canvas->setSource(GlyphContinuous::SRC_TextStringRepeated);
    canvas->setMouseOperationEnabled(false);
    canvas->setSourceText(sourceTextEdit_->toPlainText());

    canvas->installEventFilter(this);
  }
  sourceTextChanged();
}


void
ComperatorTab::forceEqualWidths()
{
  if (canvas_.empty())
    return;

  // We need to keep the columns strictly equally wide, so we need to compensate
  // the remainders when the tab width can't be evenly divided.
  // Since the canvases are contained within QFrames, we can safely set fixed
  // widths to them without messying up with the QGridLayout layouting.
  // Using the first canvas as the reference width.
  auto w = canvas_[0]->size().width();
  for (int i = 1; static_cast<unsigned>(i) < canvas_.size(); ++i)
    canvas_[i]->setFixedWidth(w);
}


void
ComperatorTab::reloadStringAndRepaint()
{
  int i = 0;
  for (auto canvas : canvas_)
  {
    syncSettings(i);

    auto& sr = canvas->stringRenderer();
    sr.setCharMapIndex(charMapSelector_->currentCharMapIndex(), -1);
    sr.reloadAll();
    i++;
  }
  repaintGlyph();
}


void
ComperatorTab::reloadGlyphsAndRepaint()
{
  int i = 0;
  for (auto canvas : canvas_)
  {
    syncSettings(i);
    canvas->stringRenderer().reloadGlyphs();
    i++;
  }
  repaintGlyph();
}


void
ComperatorTab::sourceTextChanged()
{
  for (auto canvas : canvas_)
    canvas->setSourceText(sourceTextEdit_->toPlainText());
  reloadStringAndRepaint();
}


void
ComperatorTab::syncSettings(int index)
{
  if (index < 0 || static_cast<unsigned>(index) >= settingPanels_.size())
    return;

  auto settingPanel = settingPanels_[index];
  settingPanel->applyDelayedSettings();
  settingPanel->syncSettings();

  if (static_cast<unsigned>(index) >= canvas_.size())
    return;

  auto canvas = canvas_[index];
  canvas->stringRenderer().setKerning(settingPanel->kerningEnabled());
  canvas->stringRenderer().setLsbRsbDelta(settingPanel->lsbRsbDeltaEnabled());
}


namespace
{
const char* ComparatorDefaultText
    = "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Cras sit amet"
      " dui.  Nam sapien. Fusce vestibulum ornare metus. Maecenas ligula orci,"
      " consequat vitae, dictum nec, lacinia non, elit. Aliquam iaculis"
      " molestie neque. Maecenas suscipit felis ut pede convallis malesuada."
      " Aliquam erat volutpat. Nunc pulvinar condimentum nunc. Donec ac sem vel"
      " leo bibendum aliquam. Pellentesque habitant morbi tristique senectus et"
      " netus et malesuada fames ac turpis egestas.\n"
      "\n"
      "Sed commodo. Nulla ut libero sit amet justo varius blandit. Mauris vitae"
      " nulla eget lorem pretium ornare. Proin vulputate erat porta risus."
      " Vestibulum malesuada, odio at vehicula lobortis, nisi metus hendrerit"
      " est, vitae feugiat quam massa a ligula. Aenean in tellus. Praesent"
      " convallis. Nullam vel lacus.  Aliquam congue erat non urna mollis"
      " faucibus. Morbi vitae mauris faucibus quam condimentum ornare. Quisque"
      " sit amet augue. Morbi ullamcorper mattis enim. Aliquam erat volutpat."
      " Morbi nec felis non enim pulvinar lobortis.  Ut libero. Nullam id orci"
      " quis nisl dapibus rutrum. Suspendisse consequat vulputate leo. Aenean"
      " non orci non tellus iaculis vestibulum. Sed neque.\n"
      "\n";
}


// end of comparator.cpp
