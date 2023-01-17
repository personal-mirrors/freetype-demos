// comparator.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "comparator.hpp"

#include <QScrollBar>


namespace
{
extern const char* ComparatorDefaultText;
}


ComparatorTab::ComparatorTab(QWidget* parent,
                             Engine* engine)
: QWidget(parent),
  engine_(engine)
{
  createLayout();
  createConnections();
  setupCanvases();
}


ComparatorTab::~ComparatorTab()
{
}


void
ComparatorTab::repaintGlyph()
{
  sizeSelector_->applyToEngine(engine_);

  int i = 0;
  for (auto canvas : canvas_)
  {
    applySettings(i);
    // No cache here, because when switching between columns the hinting
    // mode or enabling of embedded bitmaps may differ.
    canvas->stringRenderer().reloadGlyphs();
    canvas->purgeCache();
    canvas->repaint();
    i++;
  }
}


void
ComparatorTab::reloadFont()
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
ComparatorTab::eventFilter(QObject* watched,
                           QEvent* event)
{
  auto ptr = qobject_cast<GlyphContinuous*>(watched);
  if (ptr && event->type() == QEvent::Resize)
    return true;
  return QWidget::eventFilter(watched, event);
}


void
ComparatorTab::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  forceEqualWidths();
  repaintGlyph();
}


void
ComparatorTab::createLayout()
{
  sizeSelector_ = new FontSizeSelector(this, true, true);
  charMapLabel_ = new QLabel(tr("Char Map:"), this);
  charMapSelector_ = new CharMapComboBox(this, engine_, false);
  sourceTextEdit_ = new QPlainTextEdit(QString(ComparatorDefaultText), this);

  sizeSelector_->installEventFilterForWidget(this);

  for (int i = 0; i < ColumnWidth; ++i)
  {
    auto frame = new QFrame(this);
    auto canvas = new GlyphContinuous(frame, engine_);
    auto settingPanel = new SettingPanel(this, engine_, true);
    settingPanel->setDefaultsPreset(i);

    sizeSelector_->installEventFilterForWidget(canvas);

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

  // Tooltips
  sourceTextEdit_->setToolTip(tr("Source text for previewing"));

  // Layouting
  layout_ = new QGridLayout;

  sourceLayout_ = new QVBoxLayout;
  sourceLayout_->addWidget(sizeSelector_);
  sourceLayout_->addWidget(charMapLabel_);
  sourceLayout_->addWidget(charMapSelector_);
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
ComparatorTab::createConnections()
{
  connect(sizeSelector_, &FontSizeSelector::valueChanged,
          this, &ComparatorTab::reloadGlyphsAndRepaint);
  connect(sourceTextEdit_, &QPlainTextEdit::textChanged,
          this, &ComparatorTab::sourceTextChanged);
  connect(charMapSelector_,
          QOverload<int>::of(&CharMapComboBox::currentIndexChanged),
          this, &ComparatorTab::reloadStringAndRepaint);

  for (auto panel : settingPanels_)
  {
    // We're treating the two events identically because we need to do a
    // complete flush anyway.
    connect(panel, &SettingPanel::repaintNeeded,
            this, &ComparatorTab::repaintGlyph);
    connect(panel, &SettingPanel::fontReloadNeeded,
            this, &ComparatorTab::repaintGlyph);
  }

  for (auto canvas : canvas_)
  {
    connect(canvas, &GlyphContinuous::wheelZoom,
            this, &ComparatorTab::wheelZoom);
    connect(canvas, &GlyphContinuous::wheelResize,
            this, &ComparatorTab::wheelResize);
  }
}


void
ComparatorTab::setupCanvases()
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
ComparatorTab::forceEqualWidths()
{
  if (canvas_.empty())
    return;

  // We need to keep the columns strictly equally wide, so we need to
  // compensate the remainders when the tab width can't be evenly divided.
  // Since the canvases are contained within `QFrame`s, we can safely set
  // fixed widths to them without messing up with the `QGridLayout`
  // layouting.  Using the first canvas as the reference width.
  auto w = canvas_[0]->size().width();
  for (int i = 1; static_cast<unsigned>(i) < canvas_.size(); ++i)
    canvas_[i]->setFixedWidth(w);
}


void
ComparatorTab::reloadStringAndRepaint()
{
  int i = 0;
  for (auto canvas : canvas_)
  {
    applySettings(i);

    auto& sr = canvas->stringRenderer();
    sr.setCharMapIndex(charMapSelector_->currentCharMapIndex(), -1);
    sr.reloadAll();
    i++;
  }
  repaintGlyph();
}


void
ComparatorTab::reloadGlyphsAndRepaint()
{
  int i = 0;
  for (auto canvas : canvas_)
  {
    applySettings(i);
    canvas->stringRenderer().reloadGlyphs();
    i++;
  }
  repaintGlyph();
}


void
ComparatorTab::sourceTextChanged()
{
  for (auto canvas : canvas_)
    canvas->setSourceText(sourceTextEdit_->toPlainText());
  reloadStringAndRepaint();
}


void
ComparatorTab::applySettings(int index)
{
  if (index < 0 || static_cast<unsigned>(index) >= settingPanels_.size())
    return;

  auto settingPanel = settingPanels_[index];
  settingPanel->applyDelayedSettings();
  settingPanel->applySettings();

  if (static_cast<unsigned>(index) >= canvas_.size())
    return;

  auto canvas = canvas_[index];
  canvas->setScale(sizeSelector_->zoomFactor());
  canvas->stringRenderer().setKerning(settingPanel->kerningEnabled());
  canvas->stringRenderer().setLsbRsbDelta(settingPanel->lsbRsbDeltaEnabled());
}


void
ComparatorTab::wheelResize(int steps)
{
  sizeSelector_->handleWheelResizeBySteps(steps);
}


void
ComparatorTab::wheelZoom(int steps)
{
  sizeSelector_->handleWheelZoomBySteps(steps);
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
