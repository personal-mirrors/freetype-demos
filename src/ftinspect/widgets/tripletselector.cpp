// tripletselector.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "tripletselector.hpp"
#include "../engine/engine.hpp"

#include <functional>


TripletSelector::TripletSelector(QWidget* parent,
                                 Engine* engine)
: QWidget(parent),
  engine_(engine)
{
  createLayout();
  createConnections();
  checkButtons();
}


TripletSelector::~TripletSelector()
{
}


void
TripletSelector::repopulateFonts()
{
  auto oldSize = fontComboBox_->count();
  auto oldIndex = fontComboBox_->currentIndex();

  {
    QSignalBlocker blk(fontComboBox_);
    QSignalBlocker blk2(faceComboBox_);
    QSignalBlocker blk3(niComboBox_);
    fontComboBox_->clear();

    auto& ffm = engine_->fontFileManager();
    auto newSize = ffm.size();
    for (int i = 0; i < newSize; i++)
    {
      auto& info = ffm[i];
      auto name = info.filePath();
      auto displayName = info.fileName();
      if (info.isSymbolicLink())
        displayName += " [symlink]";

      fontComboBox_->addItem(displayName, name);
    }

    if (newSize > oldSize)
    {
      // If we have new fonts, set the current index to the first new one.
      fontComboBox_->setCurrentIndex(oldSize);
    }
    else if (newSize < oldSize)
    {
      if (oldIndex >= newSize)
        oldIndex = newSize - 1;
      if (oldIndex < 0)
        oldIndex = -1;
      fontComboBox_->setCurrentIndex(oldIndex);
    }
    else // Failed to open, retain.
      fontComboBox_->setCurrentIndex(oldIndex);

    // Note: no signal is emitted from any combobox until this block ends.
  }

  // This checks buttons & reloads the triplet.
  repopulateFaces();
}


void
TripletSelector::repopulateFaces(bool fontSwitched)
{
  // Avoid unnecessary recreating to reduce interruption of user operations.
  auto needToRecreate = fontSwitched;
  auto oldSize = faceComboBox_->count();

  auto fontIndex = fontComboBox_->currentIndex();
  auto newSize = engine_->numberOfFaces(fontIndex);

  if (fontIndex < 0 || newSize < 0)
  {
    // Clear and go.
    faceComboBox_->clear();
    // This checks buttons & reloads the triplet.
    repopulateNamedInstances(fontSwitched);
    return;
  }

  if (newSize != oldSize)
    needToRecreate = true;

  std::vector<QString> newFaces;
  newFaces.reserve(newSize);
  for (long i = 0; i < newSize; i++)
  {
    newFaces.emplace_back(engine_->namedInstanceName(fontIndex, i, 0));
    if (!needToRecreate && newFaces[i] != faceComboBox_->itemData(i))
      needToRecreate = true;
  }

  if (!needToRecreate)
  {
    // No need to refresh the combobox.
    // This checks buttons & reloads the triplet.
    repopulateNamedInstances(fontSwitched);
    return;
  }

  {
    QSignalBlocker blk2(faceComboBox_);
    QSignalBlocker blk3(niComboBox_);
    faceComboBox_->clear();

    for (long i = 0; i < newSize; i++)
    {
      auto& name = newFaces[i];
      auto displayName = QString("%1: %2").arg(i).arg(name);
      faceComboBox_->addItem(displayName, name);
    }

    faceComboBox_->setCurrentIndex(0);
    // Note: no signal gets emitted from any combobox until this block ends.
  }

  // This checks buttons & reloads the triplet.
  repopulateNamedInstances(true);
}


void
TripletSelector::repopulateNamedInstances(bool fontSwitched)
{
  // Avoid unnecessary recreating to reduce interruption of user operations.
  // Similar to `repopulateFaces`.
  auto needToRecreate = fontSwitched;
  auto oldSize = niComboBox_->count();

  auto fontIndex = fontComboBox_->currentIndex();
  auto faceIndex = faceComboBox_->currentIndex();
  auto newSize = engine_->numberOfNamedInstances(fontIndex, faceIndex);

  if (fontIndex < 0 || faceIndex < 0 || newSize < 0)
  {
    // Clear and go, and don't forget checking buttons and loading triplet.
    niComboBox_->clear();
    checkButtons();
    loadTriplet();
    return;
  }

  if (newSize != oldSize)
    needToRecreate = true;

  std::vector<QString> newFaces;
  newFaces.reserve(newSize);
  for (long i = 0; i < newSize; i++)
  {
    newFaces.emplace_back(engine_->namedInstanceName(fontIndex, faceIndex, i));
    if (!needToRecreate && newFaces[i] != niComboBox_->itemData(i))
      needToRecreate = true;
  }

  niComboBox_->setEnabled(newSize > 1);

  if (!needToRecreate)
  {
    // No need to refresh the combobox.
    checkButtons();
    loadTriplet();
    return;
  }

  {
    QSignalBlocker blk3(niComboBox_);
    niComboBox_->clear();

    for (long i = 0; i < newSize; i++)
    {
      auto& name = newFaces[i];
      auto displayName = QString("%1: %2").arg(i).arg(name);
      if (i == 0)
        displayName = "* " + displayName;
      niComboBox_->addItem(displayName, name);
    }

    niComboBox_->setCurrentIndex(0);
    // Note: no signal is emitted from any combobox until this block ends.
  }

  checkButtons();
  loadTriplet();
}


void
TripletSelector::closeCurrentFont()
{
  auto idx = fontComboBox_->currentIndex();
  if (idx < 0)
    return;
  engine_->fontFileManager().remove(idx);

  // Show next font after deletion, i.e., retain index if possible.
  int num = engine_->numberOfOpenedFonts();
  if (num)
  {
    if (idx >= num)
      idx = num - 1;
  }
  else
    idx = -1;

  {
    // Shut up when repopulating.
    QSignalBlocker blockerThis(this);
    QSignalBlocker blockerComboBox(fontComboBox_);
    repopulateFonts();
  }

  if (idx != -1)
    faceComboBox_->setCurrentIndex(idx);
  updateFont();
}


void
TripletSelector::updateFont()
{
  auto idx = fontComboBox_->currentIndex();
  auto num = engine_->numberOfOpenedFonts();
  if (idx < 0)
  {
    faceComboBox_->clear();
    niComboBox_->clear();

    checkButtons();
    loadTriplet();
    return;
  }

  if (num <= 0 || idx >= num)
  {
    // Out of sync: this shouldn't happen.
    repopulateFonts();
    return;
  }

  // This checks buttons & reloads the triplet.
  repopulateFaces();
}


void
TripletSelector::updateFace()
{
  auto idx = faceComboBox_->currentIndex();
  auto num = engine_->numberOfFaces(fontComboBox_->currentIndex());

  if (idx >= num)
  {
    // Out of sync.
    repopulateFaces();
    return;
  }

  // This checks buttons & reloads the triplet.
  repopulateNamedInstances();
}


void
TripletSelector::updateNI()
{
  auto idx = niComboBox_->currentIndex();
  auto num = engine_->numberOfNamedInstances(fontComboBox_->currentIndex(),
                                             faceComboBox_->currentIndex());

  if (idx >= num)
  {
    // Out of sync.
    repopulateNamedInstances();
    return;
  }

  checkButtons();
  loadTriplet();
}


void
TripletSelector::checkButtons()
{
  fontUpButton_->setEnabled(fontComboBox_->currentIndex() > 0);
  fontDownButton_->setEnabled(fontComboBox_->currentIndex()
                              < fontComboBox_->count() - 1);
  closeFontButton_->setEnabled(faceComboBox_->currentIndex() >= 0);

  faceUpButton_->setEnabled(faceComboBox_->currentIndex() > 0);
  faceDownButton_->setEnabled(faceComboBox_->currentIndex()
                              < faceComboBox_->count() - 1);

  niUpButton_->setEnabled(niComboBox_->currentIndex() > 0);
  niDownButton_->setEnabled(niComboBox_->currentIndex()
                            < niComboBox_->count() - 1);
}


void
TripletSelector::watchCurrentFont()
{
  repopulateFaces(false);
}


void
TripletSelector::createLayout()
{
  fontComboBox_ = new QComboBox(this);
  faceComboBox_ = new QComboBox(this);
  niComboBox_ = new QComboBox(this);

  fontComboBox_->setPlaceholderText(tr("No font open"));
  faceComboBox_->setPlaceholderText(tr("No face available"));
  niComboBox_->setPlaceholderText(tr("No named instance available"));

  closeFontButton_ = new QToolButton(this);
  fontUpButton_ = new QToolButton(this);
  faceUpButton_ = new QToolButton(this);
  niUpButton_ = new QToolButton(this);
  fontDownButton_ = new QToolButton(this);
  faceDownButton_ = new QToolButton(this);
  niDownButton_ = new QToolButton(this);

  closeFontButton_->setText(tr("Close"));
  fontUpButton_->setText(tr("\xE2\x86\x91"));
  faceUpButton_->setText(tr("\xE2\x86\x91"));
  niUpButton_->setText(tr("\xE2\x86\x91"));
  fontDownButton_->setText(tr("\xE2\x86\x93"));
  faceDownButton_->setText(tr("\xE2\x86\x93"));
  niDownButton_->setText(tr("\xE2\x86\x93"));

  fontComboBox_->setSizePolicy(QSizePolicy::Minimum,
                               QSizePolicy::Expanding);
  faceComboBox_->setSizePolicy(QSizePolicy::Minimum,
                               QSizePolicy::Expanding);
  niComboBox_->setSizePolicy(QSizePolicy::Minimum,
                             QSizePolicy::Expanding);
  closeFontButton_->setSizePolicy(QSizePolicy::Maximum,
                                  QSizePolicy::Expanding);

  fontUpButton_->setFixedSize(30, 30);
  faceUpButton_->setFixedSize(30, 30);
  niUpButton_->setFixedSize(30, 30);
  fontDownButton_->setFixedSize(30, 30);
  faceDownButton_->setFixedSize(30, 30);
  niDownButton_->setFixedSize(30, 30);

  // Tooltips
  fontComboBox_->setToolTip(tr("Current font"));
  faceComboBox_->setToolTip(tr("Current subfont (face)"));
  niComboBox_->setToolTip(tr(
    "Current named instance (only available for variable fonts)"));
  closeFontButton_->setToolTip(tr("Close current font"));
  fontUpButton_->setToolTip(tr("Previous font"));
  faceUpButton_->setToolTip(tr("Previous subfont (face)"));
  niUpButton_->setToolTip(tr("Previous named instance"));
  fontDownButton_->setToolTip(tr("Next font"));
  faceDownButton_->setToolTip(tr("Next subfont (face)"));
  niDownButton_->setToolTip(tr("Next named instance"));

  // Layouting
  layout_ = new QHBoxLayout;
  layout_->setSpacing(0);
  layout_->setContentsMargins(0, 0, 0, 0);

  layout_->addWidget(fontComboBox_);
  layout_->addWidget(fontUpButton_);
  layout_->addWidget(fontDownButton_);
  layout_->addWidget(closeFontButton_);
  layout_->addWidget(faceComboBox_);
  layout_->addWidget(faceUpButton_);
  layout_->addWidget(faceDownButton_);
  layout_->addWidget(niComboBox_);
  layout_->addWidget(niUpButton_);
  layout_->addWidget(niDownButton_);

  setFixedHeight(30);
  setLayout(layout_);
  layout_->setContentsMargins(0, 0, 0, 0);
}


void
TripletSelector::createConnections()
{
  connect(fontComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &TripletSelector::updateFont);
  connect(faceComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &TripletSelector::updateFace);
  connect(niComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &TripletSelector::updateNI);

  connect(closeFontButton_, &QToolButton::clicked,
          this, &TripletSelector::closeCurrentFont);
  connect(fontUpButton_, &QToolButton::clicked,
          this,
          std::bind(&TripletSelector::previousComboBoxItem, fontComboBox_));
  connect(faceUpButton_, &QToolButton::clicked,
          this,
          std::bind(&TripletSelector::previousComboBoxItem, faceComboBox_));
  connect(niUpButton_, &QToolButton::clicked,
          this,
          std::bind(&TripletSelector::previousComboBoxItem, niComboBox_));
  connect(fontDownButton_, &QToolButton::clicked,
          this,
          std::bind(&TripletSelector::nextComboBoxItem, fontComboBox_));
  connect(faceDownButton_, &QToolButton::clicked,
          this,
          std::bind(&TripletSelector::nextComboBoxItem, faceComboBox_));
  connect(niDownButton_, &QToolButton::clicked,
          this,
          std::bind(&TripletSelector::nextComboBoxItem, niComboBox_));

  connect(&engine_->fontFileManager(), &FontFileManager::currentFileChanged,
          this, &TripletSelector::watchCurrentFont);
}


void
TripletSelector::loadTriplet()
{
  // We do lazy computation of `FT_Face` objects.

  // TODO really?
  auto fontIndex = fontComboBox_->currentIndex();
  auto faceIndex = faceComboBox_->currentIndex();
  auto instanceIndex = niComboBox_->currentIndex();

  if (fontIndex >= 0 && fontIndex < engine_->numberOfOpenedFonts())
  {
    QFileInfo& fileInfo = engine_->fontFileManager()[fontIndex];
    engine_->fontFileManager().updateWatching(fontIndex);

    if (!fileInfo.exists())
    {
      // On Unix-like systems, the symlink's target gets opened; this
      // implies that deletion of a symlink doesn't make `engine->loadFont'
      // fail since it operates on a file handle pointing to the target.
      // For this reason, we remove the font to enforce a reload.
      engine_->removeFont(fontIndex, false);
    }
  }

  if (faceIndex < 0)
    faceIndex = 0;
  if (instanceIndex < 0)
    instanceIndex = 0;

  engine_->loadFont(fontIndex, faceIndex, instanceIndex);

  // TODO: This may mess up with bitmap-only fonts.
  if (!engine_->fontValid())
  {
    // There might be various reasons why the current
    // (file, face, instance) triplet is invalid or missing;
    // we thus start our timer to periodically test
    // whether the font starts working.
    if (faceIndex >= 0 && faceIndex < engine_->numberOfOpenedFonts())
      engine_->fontFileManager().timerStart();
  }

  emit tripletChanged();
}


void
TripletSelector::nextComboBoxItem(QComboBox* c)
{
  if (c->currentIndex() < 0 || c->currentIndex() >= c->count() - 1)
    return;
  // No need to handle further steps,
  // the event handler will take care of these.
  c->setCurrentIndex(c->currentIndex() + 1);
}


void
TripletSelector::previousComboBoxItem(QComboBox* c)
{
  if (c->currentIndex() <= 0)
    return;
  // No need to handle further steps,
  // the event handler will take care of these.
  c->setCurrentIndex(c->currentIndex() - 1);
}


// end of tripletselector.cpp
